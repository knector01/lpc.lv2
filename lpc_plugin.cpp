#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "lpc.h"

#define LPC_URI "http://example.com/plugins/lpc_plugin"

#define BUFFER_SIZE 2048

typedef enum {
	LPC_INPUT = 0,
	LPC_OUTPUT = 1,
	LPC_ORDER = 2,
	LPC_WHISPER = 3,
	LPC_LATENCY = 4
} PortIndex;

typedef struct {
	const float* input;
	float* output;
	const float* order;
	const float* whisper;
	float* latency;
	lpc_data lpc_instance;
	
	float* inbuffer;
	float* outbuffer;
	int buffer_pos;
	
} LPCPlugin;

static LV2_Handle instantiate(
		const LV2_Descriptor* descriptor,
		double rate,
		const char* bundle_path,
		const LV2_Feature* const* features)
{
	//std::cout << "LPC instantiate" << std::endl;
	
	LPCPlugin* lpc_plugin = (LPCPlugin*) malloc(sizeof(LPCPlugin));
	
	lpc_plugin->lpc_instance = lpc_create();
	
	lpc_plugin->inbuffer = (float*)malloc(BUFFER_SIZE*sizeof(float));
	lpc_plugin->outbuffer = (float*)malloc(BUFFER_SIZE*sizeof(float));
	
	memset(lpc_plugin->inbuffer, 0, BUFFER_SIZE * sizeof(float));
	memset(lpc_plugin->outbuffer, 0, BUFFER_SIZE * sizeof(float));
	
	lpc_plugin->buffer_pos = 0;
	
	return (LV2_Handle) lpc_plugin;
}

static void connect_port (
		LV2_Handle instance,
		uint32_t port,
		void* data)
{
	LPCPlugin* lpc_plugin = (LPCPlugin*) instance;
	
	switch ((PortIndex) port) {
		case LPC_INPUT:
			lpc_plugin->input = (const float*)data;
			break;
		case LPC_OUTPUT:
			lpc_plugin->output = (float*)data;
			break;
		case LPC_ORDER:
			lpc_plugin->order = (const float*)data;
			break;
		case LPC_WHISPER:
			lpc_plugin->whisper = (const float*)data;
			break;
		case LPC_LATENCY:
			lpc_plugin->latency = (float*)data;
			break;
	}
	
}

static void activate (LV2_Handle instance)
{
	//std::cout << "LPC activate" << std::endl;
	
	//LPCPlugin* lpc_plugin = (LPCPlugin*) instance;
}

static void run (
		LV2_Handle instance,
		uint32_t samples)
{
	LPCPlugin* lpc_plugin = (LPCPlugin*) instance;
	
	const float* const input = lpc_plugin->input;
	float* const output = lpc_plugin->output;
	
	float* inbuffer = (float*)lpc_plugin->inbuffer;
	float* outbuffer = (float*)lpc_plugin->outbuffer;
	
	const int order = (int) *(lpc_plugin->order);
	const float whisper = *(lpc_plugin->whisper);
	
	int* buffer_pos = &(lpc_plugin->buffer_pos);
	float* latency = lpc_plugin->latency;
	
	//std::cout << "run " << *buffer_pos << std::endl;
	
	for (int i = 0; i < samples; i++) {
		
		//std::cout << i << " " << *buffer_pos << std::endl;
		
		inbuffer[*buffer_pos] = input[i];
		output[i] = outbuffer[*buffer_pos];
		
		(*buffer_pos)++;
		
		if (*buffer_pos >= BUFFER_SIZE) {
			float coefs[order];
			float power;
			float pitch;
			
			lpc_analyze(lpc_plugin->lpc_instance, (float*)inbuffer,
						BUFFER_SIZE, coefs, order, &power, &pitch);
			
			// NOTE: Pitch 0: whisper, other values are wavelength in samples
			// (sample rate over frequency)
			if (whisper > 0) {
				pitch = 0.0f;
			}
			
			lpc_synthesize(lpc_plugin->lpc_instance, outbuffer,
							BUFFER_SIZE, coefs, order, power, pitch);
			
			*buffer_pos = 0;
		}
	}
	
	*latency = (float)BUFFER_SIZE;
	//std::cout << "finish " << *buffer_pos << std::endl;
}

static void deactivate (LV2_Handle instance)
{
	//std::cout << "LPC deactivate" << std::endl;
	
	//const LPCPlugin* lpc_plugin = (const LPCPlugin*) instance;
	
	
}


static void cleanup (LV2_Handle instance)
{
	//std::cout << "LPC cleanup" << std::endl;
	
	LPCPlugin* lpc_plugin = (LPCPlugin*) instance;
	
	lpc_destroy(lpc_plugin->lpc_instance);
	
	free(lpc_plugin->inbuffer);
	free(lpc_plugin->outbuffer);
	
	free(lpc_plugin);
}

static const void* extension_data(const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
	LPC_URI,
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch(index) {
		case 0: return &descriptor;
		default: return NULL;
	}
}
