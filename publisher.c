#include <dds/dds.h>
#include <stdio.h>
#include <stdlib.h>

#include <libavformat/avformat.h>

#include "VideoData.h"

#define DATA_SIZE 27


int main (int argc, char ** argv)
{
  dds_entity_t participant;
  dds_entity_t topic;
  dds_entity_t writer;
  dds_return_t rc;

  dds_qos_t *qos;

  VideoData_frame vdframe;
  char *userdata;
  uint32_t status = 0;

  int i;

  (void)argc;
  (void)argv;

  qos = dds_create_qos();


  userdata = dds_alloc(32);
  for(i = 0; i < 31; i++)
    userdata[i] = 'a' + i;
  userdata[31] = 0;
  dds_free(userdata);

  dds_qset_userdata(qos, userdata, 32);
  /* Create a Participant. */
  participant = dds_create_participant (DDS_DOMAIN_DEFAULT, qos, NULL);
  if (participant < 0)
    DDS_FATAL("dds_create_participant: %s\n", dds_strretcode(-participant));
  dds_delete_qos(qos);

  qos = dds_create_qos();
  if(qos == NULL)
    DDS_FATAL("dds create qos failed");
  dds_qset_durability(qos, DDS_DURABILITY_TRANSIENT_LOCAL);
  /* Create a Topic. */
  topic = dds_create_topic (
    participant, &VideoData_frame_desc, "VideoData_frame", qos, NULL);
  if (topic < 0)
    DDS_FATAL("dds_create_topic: %s\n", dds_strretcode(-topic));
  dds_delete_qos(qos);

  /* Create a Writer. */
  writer = dds_create_writer (participant, topic, NULL, NULL);
  if (writer < 0)
    DDS_FATAL("dds_create_writer: %s\n", dds_strretcode(-writer));

  printf("=== [Publisher]  Waiting for a reader to be discovered ...\n");
  fflush (stdout);

  /* Create a message to write. */
  vdframe.data._buffer = dds_alloc(DATA_SIZE);
  vdframe.data._length = DATA_SIZE;
  vdframe.data._release = true;
  vdframe.data._maximum = 0;

  for(i = 0; i < DATA_SIZE-1; i++)
    vdframe.data._buffer[i] = 'A' + i;

  vdframe.data._buffer[DATA_SIZE-1] = 0;

  while (true)
  {
    vdframe.pts = dds_time();
    printf("write topic, pts is %ld\n", vdframe.pts);
    rc = dds_write(writer, &vdframe);
    if(rc != DDS_RETCODE_OK)
        DDS_FATAL("dds write failed, %s\n", dds_strretcode(-rc));
    
    dds_sleepfor(DDS_SECS(1));
  }

  /* Deleting the participant will delete all its children recursively as well. */
  rc = dds_delete (participant);
  if (rc != DDS_RETCODE_OK)
    DDS_FATAL("dds_delete: %s\n", dds_strretcode(-rc));

  return EXIT_SUCCESS;
}
