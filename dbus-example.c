#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test123.h"

void query(char *param, int param2)
{
    DBusMessage *msg;
    DBusMessageIter args;
    DBusConnection *conn;
    DBusError err;
    DBusPendingCall *pending;
    int ret;
    bool stat;
    dbus_uint32_t level;

    printf("Calling remote method with %s and %d.\n", param, param2);

    // initialiset the errors
    dbus_error_init(&err);

    // connect to the system bus and check for errors
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err))
    {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn)
    {
        exit(1);
    }

    // request our name on the bus
    ret = dbus_bus_request_name(conn, "test.method.caller", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err))
    {
        fprintf(stderr, "Name Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
    {
        exit(1);
    }

    // create a new method call and check for errors
    msg = dbus_message_new_method_call("test.method.server", // target for the method call
                                       "/test/method/Object", // object to call on
                                       "test.method.Type", // interface to call on
                                       "first_func"); // method name
    if (NULL == msg)
    {
        fprintf(stderr, "Message Null\n");
        exit(1);
    }

    // append arguments
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &param))
    {
        fprintf(stderr, "Out Of Memory!\n");
        exit(1);
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &param2))
    {
        fprintf(stderr, "Out Of Memory!\n");
        exit(1);
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) // -1 is default timeout
    {
        fprintf(stderr, "Out Of Memory!\n");
        exit(1);
    }
    if (NULL == pending)
    {
        fprintf(stderr, "Pending Call Null\n");
        exit(1);
    }
    dbus_connection_flush(conn);

    printf("Request Sent\n");

    // free message
    dbus_message_unref(msg);

    // block until we recieve a reply
    dbus_pending_call_block(pending);

    // get the reply message
    msg = dbus_pending_call_steal_reply(pending);
    if (NULL == msg)
    {
        fprintf(stderr, "Reply Null\n");
        exit(1);
    }
    // free the pending message handle
    dbus_pending_call_unref(pending);

    // read the parameters
    if (!dbus_message_iter_init(msg, &args))
    {
        fprintf(stderr, "Message has no arguments!\n");
    }
    else if (DBUS_TYPE_BOOLEAN != dbus_message_iter_get_arg_type(&args))
    {
        fprintf(stderr, "Argument is not boolean!\n");
    }
    else
    {
        dbus_message_iter_get_basic(&args, &stat);
    }

    if (!dbus_message_iter_next(&args))
    {
        fprintf(stderr, "Message has too few arguments!\n");
    }
    else if (DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&args))
    {
        fprintf(stderr, "Argument is not int!\n");
    }
    else
    {
        dbus_message_iter_get_basic(&args, &level);
    }

    printf("Got Reply: %d, %d\n", stat, level);

    // free reply and close connection
    dbus_message_unref(msg);
    dbus_connection_close(conn);
}

void reply_to_method_call(DBusMessage *msg, DBusConnection *conn)
{
    DBusMessage *reply;
    DBusMessageIter args;
    dbus_bool_t stat = true;
    dbus_uint32_t level = 21614;
    dbus_uint32_t serial = 0;
    char *param = NULL;
    int param2=0;

    // read the arguments
    if (!dbus_message_iter_init(msg, &args))
    {
        fprintf(stderr, "Message has no arguments!\n");
    }
    else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
    {
        fprintf(stderr, "Argument is not string!\n");
    }
    else
    {
        dbus_message_iter_get_basic(&args, &param);
    }
    if (!dbus_message_iter_next(&args))
    {
        fprintf(stderr, "Message has too few arguments!\n");
    }
    else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
    {
        fprintf(stderr, "Argument is not int!\n");
    }
    else
    {
        dbus_message_iter_get_basic(&args, &param2);
    }

    printf("Method called with [%s] [%d].\n", param, param2);
    if (dbus_message_is_method_call(msg, "test.method.Type", "first_func")) 
    {
        printf("after Method called with [%s] [%d].\n", param, param2);
        first_func(param, param2);
    }

    // create a reply from the message
    reply = dbus_message_new_method_return(msg);

    // add the arguments to the reply
    dbus_message_iter_init_append(reply, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &stat))
    {
        fprintf(stderr, "Out Of Memory!\n");
        printf("dbus_message_iter_append_basic\(\&args, DBUS_TYPE_BOOLEAN, \&stat\)!\n");
        exit(1);
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &level))
    {
        fprintf(stderr, "Out Of Memory!\n");
        printf("dbus_message_iter_append_basic\(\&args, DBUS_TYPE_UINT32, \&level\)!\n");
        exit(1);
    }

    // send the reply && flush the connection
    if (!dbus_connection_send(conn, reply, &serial))
    {
        fprintf(stderr, "Out Of Memory!\n");
        exit(1);
    }
    dbus_connection_flush(conn);

    // free the reply
    dbus_message_unref(reply);
}

/**
 * Server that exposes a method call and waits for it to be called
 */
void listen()
{ 
    DBusMessage *msg;
    DBusMessage *reply;
    DBusMessageIter args;
    DBusConnection *conn;
    DBusError err;
    int ret;
    char *param;

    printf("Listening for method calls\n");

    // initialise the error
    dbus_error_init(&err);

    // connect to the bus and check for errors
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err))
    {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn)
    {
        fprintf(stderr, "Connection Null\n");
        exit(1);
    }

    // request our name on the bus and check for errors
    ret = dbus_bus_request_name(conn, "test.method.server", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err))
    {
        fprintf(stderr, "Name Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
    {
        fprintf(stderr, "Not Primary Owner (%d)\n", ret);
        exit(1);
    }

    // loop, testing for new messages
    while (true)
    {
        // non blocking read of the next available message
        dbus_connection_read_write(conn, 0);
        msg = dbus_connection_pop_message(conn);

        // loop again if we haven't got a message
        if (NULL == msg)
        {
            sleep(1);
            continue;
        }

        // check this is a method call for the right interface & method
        if (dbus_message_is_method_call(msg, "test.method.Type", "first_func"))
        {
            reply_to_method_call(msg, conn);
        }

        // free the message
        dbus_message_unref(msg);
    }

    // close the connection
    dbus_connection_close(conn);
}
/**
 * Connect to the DBUS bus and send a broadcast signal
 */
void sendsignal(SIG_V_ST *sigvalue)
{
    DBusMessage *msg;
    DBusMessageIter args;
    DBusMessageIter subStructIter;
    DBusMessageIter subArrayIter;
    DBusConnection *conn;
    DBusError err;
    int ret;
    dbus_uint32_t serial = 0;

    printf("Sending signal with value %s\n", sigvalue->name);

    // initialise the error value
    dbus_error_init(&err);

    // connect to the DBUS system bus, and check for errors
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err))
    {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn)
    {
        exit(1);
    }

    // register our name on the bus, and check for errors
    ret = dbus_bus_request_name(conn, "test.signal.source", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err))
    {
        fprintf(stderr, "Name Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
    {
        exit(1);
    }

    // create a signal & check for errors
    msg = dbus_message_new_signal("/test/signal/Object", // object name of the signal
                                  "test.signal.Type", // interface name of the signal
                                  "Test"); // name of the signal
    if (NULL == msg)
    {
        fprintf(stderr, "Message Null\n");
        exit(1);
    }

    // append arguments onto signal
/*
    char* byteStruct = (char *)malloc(sizeof(SIG_V_ST));
    byteStruct=(char *)(sigvalue);
*/
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_STRUCT, NULL, &subStructIter);
    dbus_message_iter_append_basic(&subStructIter, DBUS_TYPE_INT32, &(sigvalue->age));
    char *buff = sigvalue->name;
    dbus_message_iter_append_basic(&subStructIter, DBUS_TYPE_STRING, &buff);
    char buf[2]; buf[0] = DBUS_TYPE_INT32; buf[1] = '\0';
    dbus_message_iter_open_container(&subStructIter, DBUS_TYPE_ARRAY, buf,\
                                         &subArrayIter);
    //int i=10;
    //dbus_message_iter_append_basic(&subArrayIter, DBUS_TYPE_INT32,&i);
    int *pnum = sigvalue->num;
    dbus_message_iter_append_fixed_array(&subArrayIter, DBUS_TYPE_INT32, &(pnum),\
                                             sizeof(sigvalue->num) / sizeof(sigvalue->num[0]));

    dbus_message_iter_close_container(&subStructIter, &subArrayIter);
    dbus_message_iter_append_basic(&subStructIter, DBUS_TYPE_DOUBLE, &sigvalue->height);
    dbus_message_iter_close_container(&args, &subStructIter);

/*
if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &sigvalue))
    {
        fprintf(stderr, "Out Of Memory!\n");
        exit(1);
    }
*/

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial))
    {
        fprintf(stderr, "Out Of Memory!\n");
        exit(1);
    }
    dbus_connection_flush(conn);

    printf("Signal Sent\n");

    // free the message and close the connection
    dbus_message_unref(msg);
    dbus_connection_close(conn);
}

/**
 * Call a method on a remote object
 */

/**
 * Listens for signals on the bus
 */
void receive()
{
    DBusMessage *msg;
    DBusMessageIter args;
    DBusMessageIter subStructIter;
    DBusMessageIter subArrayIter;
    DBusConnection *conn;
    DBusError err;
    int ret;
    SIG_V_ST *sigv;
    sigv = (SIG_V_ST *)malloc(sizeof(SIG_V_ST));
    int byteCount;

    printf("Listening for signals\n");

    // initialise the errors
    dbus_error_init(&err);

    // connect to the bus and check for errors
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err))
    {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn)
    {
        exit(1);
    }

    // request our name on the bus and check for errors
    ret = dbus_bus_request_name(conn, "test.signal.sink", DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err))
    {
        fprintf(stderr, "Name Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret)
    {
        exit(1);
    }

    // add a rule for which messages we want to see
    dbus_bus_add_match(conn, "type='signal',interface='test.signal.Type'", &err); // see signals from the given interface
    dbus_connection_flush(conn);
    if (dbus_error_is_set(&err))
    {
        fprintf(stderr, "Match Error (%s)\n", err.message);
        exit(1);
    }
    printf("Match rule sent\n");

    // loop listening for signals being emmitted
    while (true)
    {

        // non blocking read of the next available message
        dbus_connection_read_write(conn, 0);
        msg = dbus_connection_pop_message(conn);

        // loop again if we haven't read a message
        if (NULL == msg)
        {
            sleep(1);
            continue;
        }

        // check if the message is a signal from the correct interface and with the correct name
        if (dbus_message_is_signal(msg, "test.signal.Type", "Test"))
        {

            // read the parameters
            if (!dbus_message_iter_init(msg, &args)) fprintf(stderr, "Message Has No Parameters\n");
            else
            {
                dbus_message_iter_recurse(&args, &subStructIter);
            }
            dbus_message_iter_get_basic(&subStructIter, &sigv->age);
            dbus_message_iter_next(&subStructIter);
            char *buff;
            dbus_message_iter_get_basic(&subStructIter, &buff);
            strcpy(sigv->name, buff);
            dbus_message_iter_next(&subStructIter);
            dbus_message_iter_recurse(&subStructIter, &subArrayIter);
            int *pnum = (int *)malloc(10);
            dbus_message_iter_get_fixed_array(&subArrayIter, &pnum, &byteCount);
            dbus_message_iter_next(&subStructIter);
            dbus_message_iter_get_basic(&subStructIter, &sigv->height);


            //printf("Got Signal with value %s\n", sigvalue);
            printf("message:%d,%s,%.2lf.\n", sigv->age, sigv->name, sigv->height);
            int i;
            for (i = 0; i < byteCount; ++i)
            {
                sigv->num[i] = pnum[i];
                printf("%d ", sigv->num[i]);
            }
            printf("\n");
        }

        // free the message
        dbus_message_unref(msg);
    }
    // close the connection
    dbus_connection_close(conn);
}

int main(int argc, char **argv)
{
    if (2 > argc)
    {
        printf("Syntax: dbus-example [send|receive|listen|query] [<param>]\n");
        return 1;
    }
    char *param = "no param";
    if (3 >= argc && NULL != argv[2])
    {
        param = argv[2];
    }
    if (0 == strcmp(argv[1], "send"))
    {
        SIG_V_ST st1 = { 20, "xiaoming", 1.72, { 1, 2, 3, 4, 1, 2, 1, 2, 3, 4 } };
        sendsignal(&st1);
    }
    else if (0 == strcmp(argv[1], "receive"))
    {
        receive();
    }
    else if (0 == strcmp(argv[1], "listen"))
    {
        listen();
    }
    else if (0 == strcmp(argv[1], "query"))
    {
        query(param, 129);
    }
    else
    {
        printf("Syntax: dbus-example [send|receive|listen|query] [<param>]\n");
        return 1;
    }
    return 0;
}
