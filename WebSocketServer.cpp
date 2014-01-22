#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "../lib/libwebsockets.h"
#include "WebSocketServer.h"

using namespace std;

WebSocketServer *self;

// **TODO: Should consider using templating or child classing

struct per_session_data__test {
	int number;
};

static int callback_test(   struct libwebsocket_context *context, 
                            struct libwebsocket *wsi, 
                            enum libwebsocket_callback_reasons reason, 
                            void *user, 
                            void *in, 
                            size_t len )
{
    int n, m;
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 512 + LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
	struct per_session_data__test *pss = (struct per_session_data__test *)user;
    
	switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            lwsl_notice( "callback_dumb_increment: LWS_CALLBACK_ESTABLISHED\n" );
    //        char buffer[25];
    //        sprintf( buffer, "[mnisjk] socket: %d\n", libwebsocket_get_socket_fd( wsi ) );
    //		lwsl_notice( buffer );
            pss->number = 0;
            
            // **SAVE: below this line
            self->onConnect( libwebsocket_get_socket_fd( wsi ) );
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            //lwsl_notice( "callback_dumb_increment: LWS_CALLBACK_SERVER_WRITEABLE\n" );
            n = sprintf((char *)p, "%d", pss->number++);
            m = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
            if (m < n) {
                lwsl_err("ERROR %d writing to di socket\n", n);
                throw "Error writing to socket";
            }
            if (false && pss->number == 50) {
                lwsl_info("close tesing limit, closing\n");
                throw "Close socket testing";
            }
            break;

        case LWS_CALLBACK_RECEIVE:
            lwsl_notice( "callback_dumb_increment: LWS_CALLBACK_RECEIVE\n" );
            lwsl_notice( "Received:\n" );
            lwsl_notice( (const char *)in );
            //m = libwebsocket_write(wsi, (unsigned char *)"test123", strlen("test123"), LWS_WRITE_TEXT);
            
            //		fprintf(stderr, "rx %d\n", (int)len);
            if (len < 6)
                break;
            if (strcmp((const char *)in, "reset\n") == 0)
                pss->number = 0;
            
            // **SAVE: below this line
            self->onMessage( libwebsocket_get_socket_fd( wsi ), string( (const char *)in ) );
            break;
        /*
         * this just demonstrates how to use the protocol filter. If you won't
         * study and reject connections based on header content, you don't need
         * to handle this callback
         */

        case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
            lwsl_notice( "callback_dumb_increment: LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION\n" );
            //dump_handshake_info(wsi);
            /* you could return non-zero here and kill the connection */
            break;

        default:
            //lwsl_notice( "callback_dumb_increment: UNKNOWN\n" );
            break;
	}

	return 0;

}


// **TODO: set these up via class calls
static struct libwebsocket_protocols protocols[] = {
	{
		"test",
		callback_test,
		sizeof(struct per_session_data__test),
		10,
	},{ NULL, NULL, 0, 0 } /* terminator */
};

void WebSocketServer::send( int socketID, string data )
{
    lwsl_notice( "Send!!\n" );
}


WebSocketServer::WebSocketServer( int port )
{
	this->port = port;
    lwsl_notice("libwebsockets test server - "
			"(C) Copyright 2010-2013 Andy Green <andy@warmcat.com> - "
						    "licensed under LGPL2.1\n");
    self = this;
}

WebSocketServer::~WebSocketServer( )
{

}

void WebSocketServer::run( )
{
    lwsl_notice( "Run\n" ); 
	
    //**TODO update these to more c++ things/have a config
    //**TODO take in options via command line
    const char *iface = NULL;
	int use_ssl = 0;
    char *resource_path = "blahblah";
	char cert_path[1024];
	char key_path[1024];
	int opts = 0;
	unsigned int oldus = 0;
	
	struct lws_context_creation_info info;
	memset(&info, 0, sizeof info);
	info.port = 7681;
	info.iface = iface;
	info.protocols = protocols;
#ifndef LWS_NO_EXTENSIONS
	info.extensions = libwebsocket_get_internal_extensions();
#endif
	// **TODO: update style
    if (!use_ssl) {
		info.ssl_cert_filepath = NULL;
		info.ssl_private_key_filepath = NULL;
	} else {
		if (strlen(resource_path) > sizeof(cert_path) - 32) {
			throw "resource path too long";
		}
		sprintf(cert_path, "%s/libwebsockets-test-server.pem",
								resource_path);
		if (strlen(resource_path) > sizeof(key_path) - 32) {
			throw "resource path too long";
		}
		sprintf(key_path, "%s/libwebsockets-test-server.key.pem",
								resource_path);

		info.ssl_cert_filepath = cert_path;
		info.ssl_private_key_filepath = key_path;
	}
	info.gid = -1;
	info.uid = -1;
	info.options = opts;
 
    struct libwebsocket_context *context;
    context = libwebsocket_create_context( &info );
	if( !context )
		throw "libwebsocket init failed";
    
    // Event loop
    while( 1 )
    {
        struct timeval tv;

		gettimeofday(&tv, NULL);

        /*
		 * This provokes the LWS_CALLBACK_SERVER_WRITEABLE for every
		 * live websocket connection using the DUMB_INCREMENT protocol,
		 * as soon as it can take more packets (usually immediately)
		 */
		if( ( (unsigned int)tv.tv_usec - oldus ) > 50000 ) {
			libwebsocket_callback_on_writable_all_protocol( &protocols[0] );
			oldus = tv.tv_usec;
		}

        if( libwebsocket_service( context, 50 ) < 0 )
            throw "Error polling for socket activity.";
    }
}

