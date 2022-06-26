#pragma once
#include <cstdint>

#define MD_API __stdcall
#ifdef MD_COMPILE
#define MD_EXPORT __declspec(dllexport)
#else
#define MD_EXPORT
#endif;

extern "C"
{
	typedef struct md_handle* md_handle_t;
	typedef enum
	{
		md_event_breakpoint,
	} md_event_e;
	typedef void(*md_callback_t)(md_event_e);
	typedef enum
	{
		md_register_pc = 0,
		md_register_at = 1,
		md_register_v0 = 2,
		md_register_v1 = 3,
		md_register_a0 = 4,
		md_register_a1 = 5,
		md_register_a2 = 6,
		md_register_a3 = 7,
		md_register_t0 = 8,
		md_register_t1 = 9,
		md_register_t2 = 10,
		md_register_t3 = 11,
		md_register_t4 = 12,
		md_register_t5 = 13,
		md_register_t6 = 14,
		md_register_t7 = 15,
		md_register_s0 = 16,
		md_register_s1 = 17,
		md_register_s2 = 18,
		md_register_s3 = 19,
		md_register_s4 = 20,
		md_register_s5 = 21,
		md_register_s6 = 22,
		md_register_s7 = 23,
		md_register_t8 = 24,
		md_register_t9 = 25,
		md_register_k0 = 26,
		md_register_k1 = 27,
		md_register_gp = 28,
		md_register_sp = 29,
		md_register_fp = 30,
		md_register_ra = 31,

	} md_register_e;

	typedef enum
	{
		md_status_success = 0,
		md_status_invalid_arg,
		md_status_failure,
		md_status_access_denied,
	} md_status_e;

	typedef enum
	{
		md_state_paused = 0x0,
		md_state_enabled = 0x1,
	} md_state_e;

	MD_EXPORT md_status_e MD_API md_open(md_handle_t* device);
	MD_EXPORT md_status_e MD_API md_register_callback(md_handle_t device, md_callback_t c);
	MD_EXPORT md_status_e MD_API md_unregister_callback(md_handle_t device, md_callback_t c);
	MD_EXPORT md_status_e MD_API md_read_memory(md_handle_t device, uint8_t* buffer, uint32_t count, uint32_t offset, uint32_t* readcount);
	MD_EXPORT md_status_e MD_API md_write_memory(md_handle_t device, uint8_t* buffer, uint32_t count, uint32_t offset, uint32_t* writtencount);
	MD_EXPORT md_status_e MD_API md_read_register(md_handle_t device, md_register_e r, uint32_t* value);
	MD_EXPORT md_status_e MD_API md_write_register(md_handle_t device, md_register_e r, uint32_t value);
	MD_EXPORT md_status_e MD_API md_get_state(md_handle_t device, md_state_e* state);
	MD_EXPORT md_status_e MD_API md_set_state(md_handle_t device, md_state_e state);
	MD_EXPORT void MD_API md_close(md_handle_t device);
}