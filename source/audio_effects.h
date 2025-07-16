#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace AudioEffects {
	enum {
		EFF_NONE,
		EFF_BITCRUSH,
		EFF_DESAMPLE
	};

	// Structure pour l'en-tête WAV
	struct WAVHeader {
		char riff[4] = {'R', 'I', 'F', 'F'};
		uint32_t chunkSize;
		char wave[4] = {'W', 'A', 'V', 'E'};
		char fmt[4] = {'f', 'm', 't', ' '};
		uint32_t fmtSize = 16;
		uint16_t audioFormat = 1; // PCM
		uint16_t numChannels = 1; // Mono
		uint32_t sampleRate = 24000; // Garry's Mod voice sample rate
		uint32_t byteRate;
		uint16_t blockAlign;
		uint16_t bitsPerSample = 16;
		char data[4] = {'d', 'a', 't', 'a'};
		uint32_t dataSize;
		
		WAVHeader() {
			byteRate = sampleRate * numChannels * bitsPerSample / 8;
			blockAlign = numChannels * bitsPerSample / 8;
		}
		
		void updateSizes(uint32_t audioDataSize) {
			dataSize = audioDataSize;
			chunkSize = 36 + audioDataSize;
		}
	};

	// Classe pour gérer l'enregistrement WAV
	class VoiceRecorder {
	private:
		std::ofstream file;
		WAVHeader header;
		uint32_t samplesWritten = 0;
		std::string filename;
		
	public:
		bool startRecording(int userID) {
			// Générer le nom de fichier avec UserID et timestamp
			auto now = std::chrono::system_clock::now();
			auto time_t = std::chrono::system_clock::to_time_t(now);
			
			std::stringstream ss;
			ss << "garrysmod/recordings/user_" << userID << "_" 
			   << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".wav";
			filename = ss.str();
			
			// Créer le dossier garrysmod/recordings s'il n'existe pas (cross-platform)
			try {
				std::filesystem::create_directories("garrysmod/recordings");
			} catch (const std::exception& e) {
				std::cout << "[EightBit] Error creating directory: " << e.what() << std::endl;
				return false;
			}
			
			file.open(filename, std::ios::binary);
			if (!file.is_open()) {
				return false;
			}
			
			// Écrire l'en-tête WAV (sera mis à jour à la fin)
			file.write(reinterpret_cast<const char*>(&header), sizeof(header));
			samplesWritten = 0;
			
			return true;
		}
		
		void writeAudioData(const int16_t* samples, int numSamples) {
			if (file.is_open()) {
				file.write(reinterpret_cast<const char*>(samples), numSamples * sizeof(int16_t));
				samplesWritten += numSamples;
			}
		}
		
		void stopRecording() {
			if (file.is_open()) {
				// Mettre à jour l'en-tête avec la taille correcte
				uint32_t audioDataSize = samplesWritten * sizeof(int16_t);
				header.updateSizes(audioDataSize);
				
				// Revenir au début et réécrire l'en-tête
				file.seekp(0);
				file.write(reinterpret_cast<const char*>(&header), sizeof(header));
				
				file.close();
				samplesWritten = 0;
				
				// Get absolute path for better logging
				std::filesystem::path absPath = std::filesystem::absolute(filename);
				std::cout << "[EightBit] WAV file saved: " << absPath.string() << std::endl;
			}
		}
		
		bool isRecording() const {
			return file.is_open();
		}
		
		~VoiceRecorder() {
			stopRecording();
		}
	};

	void BitCrush(uint16_t* sampleBuffer, int samples, float quant, float gainFactor) {
		for (int i = 0; i < samples; i++) {
			//Signed shorts range from -32768 to 32767
			//Let's quantize that a bit
			float f = (float)sampleBuffer[i];
			f /= quant;
			sampleBuffer[i] = (uint16_t)f;
			sampleBuffer[i] *= quant;
			sampleBuffer[i] *= gainFactor;
		}
	}

	static uint16_t tempBuf[10 * 1024];
	void Desample(uint16_t* inBuffer, int& samples, int desampleRate = 2) {
		assert(samples / desampleRate + 1 <= sizeof(tempBuf));
		int outIdx = 0;
		for (int i = 0; i < samples; i++) {
			if (i % desampleRate == 0) continue;

			tempBuf[outIdx] = inBuffer[i];
			outIdx++;
		}
		std::memcpy(inBuffer, tempBuf, outIdx * 2);
		samples = outIdx;
	}
}