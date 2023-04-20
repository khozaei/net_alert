#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#ifndef UNUSED
#define UNUSED(x) (void)x
#endif

static struct config_s {
	gint server_port;
	GString server_ip;
	gboolean server_mode;
	gboolean gui;
	GString command;
}config;

static GOptionEntry entries[] =
{
  { "port", 'p', 0, G_OPTION_ARG_INT, &config.server_port, "Server_port", NULL },
  { "destination", 'd', 0, G_OPTION_ARG_STRING, &config.server_ip, "Server ip address", NULL },
  { "server", 's', 0, G_OPTION_ARG_NONE, &config.server_mode, "Server mode", NULL },
  { "gui", 'g', 0, G_OPTION_ARG_NONE, &config.gui, "With GUI", NULL },
  { "command", 'c', 0, G_OPTION_ARG_STRING, &config.command, "Command to send to the server", NULL },
  { NULL }
};

void
write_status(uv_write_t *req, int status){
	UNUSED(req);
	if (status == 0)
		printf("replied successfully!\n");
}

void
buffer_reader(uv_stream_t *stream, ssize_t nbytes, const uv_buf_t *buf) {
	uv_buf_t reply;
	uv_write_t req;

	if (nbytes < 0)
		uv_read_stop(stream);
	printf("from client: %s\n", buf->base);
	reply.base = (char *)"reply from tcpserver\n";
	reply.len = 21;
	uv_write(&req, stream, &reply, 1, write_status);
}

void
buffer_allocator(uv_handle_t *handle, size_t sug_size, uv_buf_t *buf) {
	UNUSED(handle);
	buf->base = malloc(sug_size);
	buf->len = sug_size;
}

void
on_new_connection(uv_stream_t *server, int status) {
	if (status == -1) {
		return;
	}
	uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
	uv_tcp_init(uv_default_loop(), client);
	if (uv_accept(server, (uv_stream_t *)client) == 0)
		uv_read_start((uv_stream_t *)client, buffer_allocator, buffer_reader);
	else
		uv_read_stop((uv_stream_t *)client);
}

int
main(int argc, char **argv) {
	uv_tcp_t server;
	struct sockaddr_in bind_addr;
	int res;
	GError *error;
	GOptionContext *context;

	error = NULL;
	config.gui = FALSE;
	config.server_mode = FALSE;
	context = g_option_context_new ("net_alert: an application to alert on demand");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_add_group (context, NULL);//gtk_get_option_group (TRUE));
	if (!g_option_context_parse (context, &argc, &argv, &error)){
		g_print ("option parsing failed: %s\n", error->message);
		exit (1);
	}
	printf("port: %i\nserver address: %s\nserver mode: %i\nGUI: %i\ncommand: %s\n\n"
		, config.server_port
		, (char *)config.server_ip.str
		, config.server_mode
		, config.gui
		, (char *)config.command.str);

	uv_tcp_init(uv_default_loop(), &server);
	uv_ip4_addr("0.0.0.0", 2986, &bind_addr);
	uv_tcp_bind(&server, (const struct sockaddr *)&bind_addr, 0);
	res = uv_listen((uv_stream_t *)&server, 128, on_new_connection);
	if (res)
		return res;
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	uv_loop_close(uv_default_loop());
	return 0;
}
