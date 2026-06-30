#include <stm32f407xx.h>
int main(void){
	return 0;
}


// ISR function is void and parameters also void
void EXTI0_IRQHandler(void)
{
	// handle the interrupt
	GPIO_IRQHandling(0);
}
