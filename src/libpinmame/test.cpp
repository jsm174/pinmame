#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <chrono>
#include <thread>
#include <queue>
#include "libpinmame.h"

#include <AL/al.h>
#include <AL/alc.h>

#if defined(_WIN32) || defined(_WIN64)
#define CLEAR_SCREEN "cls"
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define CLEAR_SCREEN "clear"
#endif

typedef unsigned char UINT8;
typedef unsigned short UINT16;

#define MAX_AUDIO_BUFFERS 4
#define MAX_AUDIO_QUEUE_SIZE 10

ALuint _audioSource;
ALuint _audioBuffers[MAX_AUDIO_BUFFERS];
std::queue<void*> _audioQueue;
int _audioChannels;
int _audioSampleRate;

void DumpDmd(int index, UINT8* p_displayData, PinmameDisplayLayout* p_displayLayout) {
	for (int y = 0; y < p_displayLayout->height; y++) {
		for (int x = 0; x < p_displayLayout->width; x++) {
			UINT8 value = p_displayData[y * p_displayLayout->width + x];
			
			if (p_displayLayout->depth == 2) {
				switch(value) {
					case 0x14:
						printf("░");
						break;
					case 0x21:
						printf("▒");
						break;
					case 0x43:
						printf("▓");
						break;
					case 0x64:
						printf("▓");
						break;
				}
			}
			else {
				if (PinmameGetHardwareGen() & (SAM | SPA)) {
					switch(value) {
						case 0x00:
						case 0x14:
						case 0x19:
						case 0x1E:
							printf("░");
							break;
						case 0x23:
						case 0x28:
						case 0x2D:
						case 0x32:
							printf("▒");
							break;
						case 0x37:
						case 0x3C:
						case 0x41:
						case 0x46:
							printf("▓");
							break;
						case 0x4B:
						case 0x50:
						case 0x5A:
						case 0x64:
							printf("▓");
							break;
					}
				}
				else {
					switch(value) {
						case 0x00:
						case 0x1E:
						case 0x23:
						case 0x28:
							printf("░");
							break;
						case 0x2D:
						case 0x32:
						case 0x37:
						case 0x3C:
							printf("▒");
							break;
						case 0x41:
						case 0x46:
						case 0x4B:
						case 0x50:
							printf("▓");
							break;
						case 0x55:
						case 0x5A:
						case 0x5F:
						case 0x64:
							printf("▓");
							break;
					}
				}
			}
		}

		printf("\n");
	}
}

void DumpAlphanumeric(int index, UINT16* p_displayData, PinmameDisplayLayout* p_displayLayout) {
	char output[8][512] = {
		{ '\0' },
		{ '\0' },
		{ '\0' },
		{ '\0' },
		{ '\0' },
		{ '\0' },
		{ '\0' },
		{ '\0' }
	};

	for (int pos = 0; pos < p_displayLayout->length; pos++) {
		const UINT16 value = *(p_displayData++);

		char segments_16c[8][10] = {
			{ " AAAAA   " },
			{ "FI J KB  " },
			{ "F IJK B  " },
			{ " GG LL   " },
			{ "E ONM C  " },
			{ "EO N MC P" }, 
			{ " DDDDD  H" },
			{ "       H " },
		};

		char segments_16s[8][10] = {
			{ " AA BB   " },
			{ "HI J KC  " },
			{ "H IJK C  " },
			{ " PP LL   " },
			{ "G ONM D  " },
			{ "GO N MD  " },
			{ " FF EE   " },
			{ "         " },
		};

		char (*segments)[10] = (p_displayLayout->type == SEG16S) ? segments_16s : segments_16c;

		for (int row = 0; row < 8; row++) {
			for (int column = 0; column < 9; column++) {
				for (UINT16 bit = 0; bit < 16; bit++) {
					if (segments[row][column] == ('A' + bit)) {
						segments[row][column] = (value & (1 << bit)) ? '*' : ' ';
						break;
					}
				}
			}

			strcat(output[row], segments[row]);
			strcat(output[row], " "); 
		}
	}

	for (int row = 0; row < 8; row++) {
		printf("%s\n", output[row]);
	}
}

void CALLBACK Game(PinmameGame* game) {
	printf("Game(): name=%s, description=%s, manufacturer=%s, year=%s, flags=%lu, found=%d\n",
		game->name, game->description, game->manufacturer, game->year, (unsigned long)game->flags, game->found);
}

void CALLBACK OnStateUpdated(int state) {
	printf("OnStateUpdated(): state=%d\n", state);

	if (!state) {
		exit(1);
	}
	else {
		PinmameMechConfig mechConfig;
		memset(&mechConfig, 0, sizeof(mechConfig));

		mechConfig.sol1 = 11;
		mechConfig.length = 240;
		mechConfig.steps = 240;
		mechConfig.type = NONLINEAR | REVERSE | ONESOL;
		mechConfig.sw[0].swNo = 32;
		mechConfig.sw[0].startPos = 0;
		mechConfig.sw[0].endPos = 5;

		PinmameSetMech(0, &mechConfig);
	}
}

void CALLBACK OnDisplayAvailable(int index, int displayCount, PinmameDisplayLayout* p_displayLayout) {
	printf("OnDisplayAvailable(): index=%d, displayCount=%d, type=%d, top=%d, left=%d, width=%d, height=%d, depth=%d, length=%d\n",
		index,
		displayCount,
		p_displayLayout->type,
		p_displayLayout->top,
		p_displayLayout->left,
		p_displayLayout->width,
		p_displayLayout->height,
		p_displayLayout->depth,
		p_displayLayout->length);
}

void CALLBACK OnDisplayUpdated(int index, void* p_displayData, PinmameDisplayLayout* p_displayLayout) {
	printf("OnDisplayUpdated(): index=%d, type=%d, top=%d, left=%d, width=%d, height=%d, depth=%d, length=%d\n",
		index,
		p_displayLayout->type,
		p_displayLayout->top,
		p_displayLayout->left,
		p_displayLayout->width,
		p_displayLayout->height,
		p_displayLayout->depth,
		p_displayLayout->length);

	if ((p_displayLayout->type & DMD) == DMD) {
		DumpDmd(index, (UINT8*)p_displayData, p_displayLayout);
	}
	else {
		DumpAlphanumeric(index, (UINT16*)p_displayData, p_displayLayout);
	}
}

int CALLBACK OnAudioAvailable(PinmameAudioInfo* p_audioInfo) {
	printf("OnAudioAvailable(): format=%d, channels=%d, sampleRate=%.2f, framesPerSecond=%.2f, samplesPerFrame=%d, bufferSize=%d\n",
		p_audioInfo->format,
		p_audioInfo->channels,
		p_audioInfo->sampleRate,
		p_audioInfo->framesPerSecond,
		p_audioInfo->samplesPerFrame,
		p_audioInfo->bufferSize);

	_audioChannels = p_audioInfo->channels;
	_audioSampleRate = (int) p_audioInfo->sampleRate;

	for (int index = 0; index < MAX_AUDIO_BUFFERS; index++) {
		void* p_buffer = malloc(p_audioInfo->samplesPerFrame * _audioChannels * sizeof(int16_t));
		memset(p_buffer, 0, p_audioInfo->samplesPerFrame * _audioChannels * sizeof(int16_t));

		alBufferData(_audioBuffers[index],
			_audioChannels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
			p_buffer, p_audioInfo->samplesPerFrame * _audioChannels * sizeof(int16_t), _audioSampleRate);
	}

	alSourceQueueBuffers(_audioSource, MAX_AUDIO_BUFFERS, _audioBuffers);
	alSourcePlay(_audioSource);

	return p_audioInfo->samplesPerFrame;
}

int CALLBACK OnAudioUpdated(void* p_buffer, int samples) {
	if (_audioQueue.size() >= MAX_AUDIO_QUEUE_SIZE) {
		while (!_audioQueue.empty()) {
			void* p_destBuffer = _audioQueue.front();

			free(p_destBuffer);
			_audioQueue.pop();
		}
	}

	void* p_destBuffer = malloc(samples * _audioChannels * sizeof(int16_t));
	memcpy(p_destBuffer, p_buffer, samples * _audioChannels * sizeof(int16_t));

	_audioQueue.push(p_destBuffer);

	ALint buffersProcessed;
	alGetSourcei(_audioSource, AL_BUFFERS_PROCESSED, &buffersProcessed);

	if (buffersProcessed <= 0) {
		return samples;
	}

	while (buffersProcessed > 0) {
		ALuint buffer = 0;
		alSourceUnqueueBuffers(_audioSource, 1, &buffer);

		if (_audioQueue.size() > 0) {
			void* p_destBuffer = _audioQueue.front();

			alBufferData(buffer,
				_audioChannels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
				p_destBuffer, samples * _audioChannels * sizeof(int16_t), _audioSampleRate);

			free(p_destBuffer);
			_audioQueue.pop();
		}

		alSourceQueueBuffers(_audioSource, 1, &buffer);
		buffersProcessed--;
	}

	ALint state;
	alGetSourcei(_audioSource, AL_SOURCE_STATE, &state);

	if (state != AL_PLAYING) {
		alSourcePlay(_audioSource);
	}

	return samples;
}

void CALLBACK OnSolenoidUpdated(int solenoid, int isActive) {
	printf("OnSolenoidUpdated: solenoid=%d, isActive=%d\n", solenoid, isActive);
}

void CALLBACK OnMechAvailable(int mechNo, PinmameMechInfo* p_mechInfo) {
	printf("OnMechAvailable: mechNo=%d, type=%d, length=%d, steps=%d, pos=%d, speed=%d\n",
		mechNo,
		p_mechInfo->type,
		p_mechInfo->length,
		p_mechInfo->steps,
		p_mechInfo->pos,
		p_mechInfo->speed);
}

void CALLBACK OnMechUpdated(int mechNo, PinmameMechInfo* p_mechInfo) {
	printf("OnMechUpdated: mechNo=%d, type=%d, length=%d, steps=%d, pos=%d, speed=%d\n",
		mechNo,
		p_mechInfo->type,
		p_mechInfo->length,
		p_mechInfo->steps,
		p_mechInfo->pos,
		p_mechInfo->speed);
}

void CALLBACK OnConsoleDataUpdated(void* p_data, int size) {
	printf("OnConsoleDataUpdated: size=%d\n", size);
}

int CALLBACK IsKeyPressed(PINMAME_KEYCODE keycode) {
	return 0;
}

int main(int, char**) {
	system(CLEAR_SCREEN);

	const ALCchar *defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	ALCdevice *device = alcOpenDevice(defaultDeviceName);

	ALCcontext *context = alcCreateContext(device, NULL);
	alcMakeContextCurrent(context);

	alGenSources((ALuint)1, &_audioSource);
	alGenBuffers(MAX_AUDIO_BUFFERS, _audioBuffers);

	PinmameConfig config = {
		AUDIO_FORMAT_INT16,
		44100,
		"",
		&OnStateUpdated,
		&OnDisplayAvailable,
		&OnDisplayUpdated,
		&OnAudioAvailable,
		&OnAudioUpdated,
		&OnMechAvailable,
		&OnMechUpdated,
		&OnSolenoidUpdated,
		&OnConsoleDataUpdated,
		&IsKeyPressed
	};

	#if defined(_WIN32) || defined(_WIN64)
		snprintf((char*)config.vpmPath, MAX_PATH, "%s%s\\pinmame\\", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
	#else
		snprintf((char*)config.vpmPath, MAX_PATH, "%s/.pinmame/", getenv("HOME"));
	#endif

	PinmameSetConfig(&config);

	PinmameSetHandleKeyboard(0);
	PinmameSetHandleMechanics(0);

	PinmameGetGames(&Game);
	PinmameGetGame("fourx4", &Game);

	//PinmameRun("mm_109c");
	//PinmameRun("fh_906h");
	//PinmameRun("hh7");
	//PinmameRun("rescu911");
	//PinmameRun("tf_180h");
	//PinmameRun("flashgdn");
	//PinmameRun("fourx4");
	//PinmameRun("ripleys");
	//PinmameRun("fh_l9");
	//PinmameRun("acd_168hc");
	//PinmameRun("snspares");

	if (PinmameRun("t2_l8") == OK) {
		while (1) {
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
	}

	return 0;
}
