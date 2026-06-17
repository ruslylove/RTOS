#ifndef STACK_SIZING_H
#define STACK_SIZING_H

void vStaticAllocTask(void *pvParameters); /* uses static stack buffer */

/* Returns the minimum stack depth (words) ever seen for a given task handle */
UBaseType_t get_high_watermark(TaskHandle_t h);

#endif
