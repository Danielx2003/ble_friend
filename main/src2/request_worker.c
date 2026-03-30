#include "request2.h"
#include "request_worker2.h"

#define BATCH_TIMEOUT_MS 5000

static request_lost_payload_t lost_batch[MAX_BATCH_ITEMS];
static size_t batch_count = 0;

void request_worker_task(void *param)
{
  request_work_item_t item;
	TickType_t last_flush = xTaskGetTickCount();

  while(1) {
	  if(xQueueReceive(request_worker_queue, &item, portMAX_DELAY)) {
			switch (item.type)
			{
				case REQUEST_WORKER_EVENT_UPLOAD_LOST_LOCATION:
					lost_batch[batch_count++] = item.lost_payload;
	
					if (batch_count >= MAX_BATCH_ITEMS) {
				    upload_lost_batch(lost_batch, batch_count);
				    batch_count = 0;
				    last_flush = xTaskGetTickCount();
					}
					break;
			}
    }
		if (batch_count > 0 &&
		    (xTaskGetTickCount() - last_flush) >= pdMS_TO_TICKS(BATCH_TIMEOUT_MS)) {

		    upload_lost_batch(lost_batch, batch_count);
		    batch_count = 0;
		    last_flush = xTaskGetTickCount();
		}
  }
}