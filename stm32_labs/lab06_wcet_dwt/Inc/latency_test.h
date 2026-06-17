#ifndef LATENCY_TEST_H
#define LATENCY_TEST_H

void vWcetTask     (void *pvParameters);  /* measures workload WCET */
void vLatencyTask  (void *pvParameters);  /* measures task-switch + semaphore latency */
void vReportTask   (void *pvParameters);  /* prints results every 5 s */

#endif
