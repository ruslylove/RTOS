#ifndef OVERFLOW_DEMO_H
#define OVERFLOW_DEMO_H

void vSafeTask    (void *pvParameters);  /* well-sized, prints watermark */
void vDangerTask  (void *pvParameters);  /* grows stack via recursion     */
void vMonitorTask (void *pvParameters);  /* reports high-water marks      */

#endif
