/*******************************************************************************
 (c) 2005-2014 Copyright, Real-Time Innovations, Inc.  All rights reserved.
 RTI grants Licensee a license to use, modify, compile, and create derivative
 works of the Software.  Licensee has the right to distribute object form only
 for use with RTI products.  The Software is provided "as is", with no warranty
 of any type, including any warranty for fitness for any purpose. RTI is under
 no obligation to maintain or support the Software.  RTI shall not be liable for
 any incidental or consequential damages arising out of the use or inability to
 use the software.
 ******************************************************************************/
/* sequences_publisher.cxx

   A publication of data of type sequences

   This file is derived from code automatically generated by the rtiddsgen
   command:

   rtiddsgen -language C++ -example <arch> sequences.idl

   Example publication of type sequences automatically generated by
   'rtiddsgen'. To test them follow these steps:

   (1) Compile this file and the example subscription.

   (2) Start the subscription with the command
       objs/<arch>/sequences_subscriber <domain_id> <sample_count>

   (3) Start the publication with the command
       objs/<arch>/sequences_publisher <domain_id> <sample_count>

   (4) [Optional] Specify the list of discovery initial peers and
       multicast receive addresses via an environment variable or a file
       (in the current working directory) called NDDS_DISCOVERY_PEERS.

   You can run any number of publishers and subscribers programs, and can
   add and remove them dynamically from the domain.


   Example:

       To run the example application on domain <domain_id>:

       On Unix:

       objs/<arch>/sequences_publisher <domain_id> o
       objs/<arch>/sequences_subscriber <domain_id>

       On Windows:

       objs\<arch>\sequences_publisher <domain_id>
       objs\<arch>\sequences_subscriber <domain_id>


modification history
------------ -------
*/

#include <stdio.h>
#include <stdlib.h>
#ifdef RTI_VX653
    #include <vThreadsData.h>
#endif
#include "ndds/ndds_cpp.h"
#include "sequences.h"
#include "sequencesSupport.h"

/* Delete all entities */
static int publisher_shutdown(DDSDomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = participant->delete_contained_entities();
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_contained_entities error %d\n", retcode);
            status = -1;
        }

        retcode = DDSTheParticipantFactory->delete_participant(participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_participant error %d\n", retcode);
            status = -1;
        }
    }

    /* RTI Connext provides finalize_instance() method on
       domain participant factory for people who want to release memory used
       by the participant factory. Uncomment the following block of code for
       clean destruction of the singleton. */
    /*
        retcode = DDSDomainParticipantFactory::finalize_instance();
        if (retcode != DDS_RETCODE_OK) {
            printf("finalize_instance error %d\n", retcode);
            status = -1;
        }
    */

    return status;
}

extern "C" int publisher_main(int domainId, int sample_count)
{
    DDSDomainParticipant *participant = NULL;
    DDSPublisher *publisher = NULL;
    DDSTopic *topic = NULL;
    DDSDataWriter *writer = NULL;
    sequencesDataWriter *sequences_writer = NULL;
    /* We define two instances to illustrate the different memory use of
     * sequences: owner_instance and borrower_instance. */
    sequences *owner_instance = NULL;
    sequences *borrower_instance = NULL;
    DDS_InstanceHandle_t owner_instance_handle = DDS_HANDLE_NIL;
    DDS_InstanceHandle_t borrower_instance_handle = DDS_HANDLE_NIL;
    DDS_ReturnCode_t retcode;
    const char *type_name = NULL;
    int count = 0;
    /* Send a new sample every second */
    DDS_Duration_t send_period = { 1, 0 };

    /* To customize participant QoS, use
       the configuration file USER_QOS_PROFILES.xml */
    participant = DDSTheParticipantFactory->create_participant(
            domainId,
            DDS_PARTICIPANT_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        printf("create_participant error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize publisher QoS, use
       the configuration file USER_QOS_PROFILES.xml */
    publisher = participant->create_publisher(
            DDS_PUBLISHER_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (publisher == NULL) {
        printf("create_publisher error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* Register type before creating topic */
    type_name = sequencesTypeSupport::get_type_name();
    retcode = sequencesTypeSupport::register_type(participant, type_name);
    if (retcode != DDS_RETCODE_OK) {
        printf("register_type error %d\n", retcode);
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize topic QoS, use
       the configuration file USER_QOS_PROFILES.xml */
    topic = participant->create_topic(
            "Example sequences",
            type_name,
            DDS_TOPIC_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (topic == NULL) {
        printf("create_topic error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize data writer QoS, use
       the configuration file USER_QOS_PROFILES.xml */
    writer = publisher->create_datawriter(
            topic,
            DDS_DATAWRITER_QOS_DEFAULT,
            NULL /* listener */,
            DDS_STATUS_MASK_NONE);
    if (writer == NULL) {
        printf("create_datawriter error\n");
        publisher_shutdown(participant);
        return -1;
    }
    sequences_writer = sequencesDataWriter::narrow(writer);
    if (sequences_writer == NULL) {
        printf("DataWriter narrow error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* Here we define two instances: owner_instance and borrower_instance. */

    /* owner_instance.data uses its own memory, as by default, a sequence
     * you create owns its memory unless you explicitly loan memory of your
     * own to it.
     */
    owner_instance = sequencesTypeSupport::create_data();
    if (owner_instance == NULL) {
        printf("sequencesTypeSupport::create_data error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* borrower_instance.data "borrows" memory from a buffer of shorts,
     * previously allocated, using DDS_ShortSeq_loan_contiguous(). */
    borrower_instance = sequencesTypeSupport::create_data();
    if (borrower_instance == NULL) {
        printf("sequencesTypeSupport::create_data error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* If we want borrower_instance.data to loan a buffer of shorts, first we
     * have to allocate the buffer. Here we allocate a buffer of
     * MAX_SEQUENCE_LEN. */
    short *short_buffer = new short[MAX_SEQUENCE_LEN];

    /* Before calling loan_contiguous(), we need to set Seq.maximum to
     * 0, i.e., the sequence won't have memory allocated to it. */
    borrower_instance->data.maximum(0);

    /* Now that the sequence doesn't have memory allocated to it, we can use
     * loan_contiguous() to loan short_buffer to borrower_instance.
     * We set the allocated number of elements to MAX_SEQUENCE_LEN, the size of
     * the buffer and the maximum size of the sequence as we declared in the
     * IDL. */
    bool return_result = borrower_instance->data.loan_contiguous(
            short_buffer,     // Pointer to the sequence
            0,                // Initial Length
            MAX_SEQUENCE_LEN  // New maximum
    );
    if (return_result != DDS_BOOLEAN_TRUE) {
        printf("loan_contiguous error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* Before starting to publish samples, set the instance id of each
     * instance*/
    strcpy(owner_instance->id, "owner_instance");
    strcpy(borrower_instance->id, "borrower_instance");

    /* To illustrate the use of the sequences, in the main loop we set a
     * new sequence length every iteration to the sequences contained in
     * both instances (instance->data). The sequence length value cycles between
     * 0 and MAX_SEQUENCE_LEN. We assign a random number between 0 and 100 to
     * each sequence's elements. */
    for (count = 0; (sample_count == 0) || (count < sample_count); ++count) {
        /* We set a different sequence_length for both instances every
         * iteration. sequence_length is based on the value of count and
         * its value cycles between the values of 1 and MAX_SEQUENCE_LEN. */
        short sequence_length = (count % MAX_SEQUENCE_LEN) + 1;

        printf("Writing sequences, count %d...\n", count);

        owner_instance->count = count;
        borrower_instance->count = count;

        /* Here we set the new length of each sequence */
        owner_instance->data.length(sequence_length);
        borrower_instance->data.length(sequence_length);

        /* Now that the sequences have a new length, we assign a
         * random number between 0 and 100 to each element of
         * owner_instance->data and borrower_instance->data. */
        for (int i = 0; i < sequence_length; ++i) {
            owner_instance->data[i] = (short) (rand() / (RAND_MAX / 100));
            borrower_instance->data[i] = (short) (rand() / (RAND_MAX / 100));
        }

        /* Both sequences have the same length, so we only print the length
         * of one of them. */
        printf("Instances length = %d\n", owner_instance->data.length());


        /* Write for each instance */
        retcode =
                sequences_writer->write(*owner_instance, owner_instance_handle);
        if (retcode != DDS_RETCODE_OK) {
            printf("write error %d\n", retcode);
        }

        retcode = sequences_writer->write(
                *borrower_instance,
                borrower_instance_handle);
        if (retcode != DDS_RETCODE_OK) {
            printf("write error %d\n", retcode);
        }

        NDDSUtility::sleep(send_period);
    }


    /* Once we are done with the sequence, we unloan and free the buffer. Make
     * sure you don't call delete before unloan(); the next time you
     * access the sequence, you will be accessing freed memory. */
    return_result = borrower_instance->data.unloan();
    if (return_result != DDS_BOOLEAN_TRUE) {
        printf("unloan error \n");
    }

    delete[] short_buffer;

    /* Delete data samples */
    retcode = sequencesTypeSupport::delete_data(owner_instance);
    if (retcode != DDS_RETCODE_OK) {
        printf("sequencesTypeSupport::delete_data error %d\n", retcode);
    }

    retcode = sequencesTypeSupport::delete_data(borrower_instance);
    if (retcode != DDS_RETCODE_OK) {
        printf("sequencesTypeSupport::delete_data error %d\n", retcode);
    }

    /* Delete all entities */
    return publisher_shutdown(participant);
}

#if defined(RTI_WINCE)
int wmain(int argc, wchar_t **argv)
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */

    if (argc >= 2) {
        domainId = _wtoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = _wtoi(argv[2]);
    }

    /* Uncomment this to turn on additional logging
   NDDSConfigLogger::get_instance()->
       set_verbosity_by_category(NDDS_CONFIG_LOG_CATEGORY_API,
                                 NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
   */

    return publisher_main(domainId, sample_count);
}

#elif !(defined(RTI_VXWORKS) && !defined(__RTP__)) && !defined(RTI_PSOS)
int main(int argc, char *argv[])
{
    int domainId = 0;
    int sample_count = 0; /* infinite loop */

    if (argc >= 2) {
        domainId = atoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = atoi(argv[2]);
    }

    /* Uncomment this to turn on additional logging
    NDDSConfigLogger::get_instance()->
        set_verbosity_by_category(NDDS_CONFIG_LOG_CATEGORY_API,
                                  NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */

    return publisher_main(domainId, sample_count);
}
#endif

#ifdef RTI_VX653
const unsigned char *__ctype = *(__ctypePtrGet());

extern "C" void usrAppInit()
{
    #ifdef USER_APPL_INIT
    USER_APPL_INIT; /* for backwards compatibility */
    #endif

    /* add application specific code here */
    taskSpawn(
            "pub",
            RTI_OSAPI_THREAD_PRIORITY_NORMAL,
            0x8,
            0x150000,
            (FUNCPTR) publisher_main,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0);
}
#endif
