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
		md_register_cop0_r0 = 32,
		md_register_cop0_r1 = 33,
		md_register_cop0_r2 = 34,
		md_register_cop0_r3 = 35,
		md_register_cop0_r4 = 36,
		md_register_cop0_r5 = 37,
		md_register_cop0_r6 = 38,
		md_register_cop0_r7 = 39,
		md_register_cop0_r8 = 40,
		md_register_cop0_r9 = 41,
		md_register_cop0_r10 = 42,
		md_register_cop0_r11 = 43,
		md_register_cop0_r12 = 44,
		md_register_cop0_r13 = 45,
		md_register_cop0_r14 = 46,
		md_register_cop0_r15 = 47,
		md_register_cop0_r16 = 48,
		md_register_cop0_r17 = 49,
		md_register_cop0_r18 = 50,
		md_register_cop0_r19 = 51,
		md_register_cop0_r20 = 52,
		md_register_cop0_r21 = 53,
		md_register_cop0_r22 = 54,
		md_register_cop0_r23 = 55,
		md_register_cop0_r24 = 56,
		md_register_cop0_r25 = 57,
		md_register_cop0_r26 = 58,
		md_register_cop0_r27 = 59,
		md_register_cop0_r28 = 60,
		md_register_cop0_r29 = 61,
		md_register_cop0_r30 = 62,
		md_register_cop0_r31 = 63,

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
	MD_EXPORT md_status_e MD_API md_read_register(md_handle_t device, md_register_e r, uint8_t sel, uint32_t* value);
	MD_EXPORT md_status_e MD_API md_write_register(md_handle_t device, md_register_e r, uint8_t sel, uint32_t value);
	MD_EXPORT md_status_e MD_API md_get_state(md_handle_t device, md_state_e* state);
	MD_EXPORT md_status_e MD_API md_set_state(md_handle_t device, md_state_e state);
	MD_EXPORT void MD_API md_close(md_handle_t device);
}