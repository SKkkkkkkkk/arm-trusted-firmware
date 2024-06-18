#include <assert.h>
#include <platform_def.h>
#include <drivers/console.h>
#include <drivers/ti/uart/uart_16550.h>
#include <lib/mmio.h>

unsigned int plat_get_syscnt_freq2(void)
{
	return mmio_read_32(GENERIC_TIMER_BASE + CNTFID_OFF);
}

static console_t console;
void console_16550_with_dlf_init(void)
{
	int ret;
	ret = console_16550_register(PLAT_UART_BASE,
					PLAT_UART_CLK_IN_HZ,
					PLAT_CONSOLE_BAUDRATE,
					&console);
	if(ret != 1)
		panic();

	#define FRACTIONAL_VALUE_DELTA 625UL
	uint8_t dlf_value;
	uint64_t fractional_value = ((PLAT_UART_CLK_IN_HZ*(uint64_t)10000)/(16*PLAT_CONSOLE_BAUDRATE))%10000;
	dlf_value = fractional_value/FRACTIONAL_VALUE_DELTA;
	if((fractional_value%FRACTIONAL_VALUE_DELTA) >= (FRACTIONAL_VALUE_DELTA/2))
		++dlf_value;
	mmio_write_32(PLAT_UART_BASE + 0xc0, dlf_value);

	console_set_scope(&console, CONSOLE_FLAG_BOOT | CONSOLE_FLAG_RUNTIME);
}