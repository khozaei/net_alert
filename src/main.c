#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

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

GString *gbuffer;

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
	if (status == 0){
		if (config.server_mode)
			printf("replied to client!\n");
		else {
			uv_shutdown_t sh;
			printf("command sent!\n");
			uv_shutdown(&sh, req->handle, NULL);
		}
	}
}

void
buffer_reader(uv_stream_t *stream, ssize_t nbytes, const uv_buf_t *buf) {
	uv_buf_t reply;
	uv_write_t req;


	if (nbytes <= 0)
		uv_read_stop(stream);
	for (int i = 0; i< nbytes; i++){
		if (g_ascii_isalpha(buf->base[i]) || buf->base[i] == '\n' || buf->base[i] == '\r')
			g_string_append_c(gbuffer, buf->base[i]);
	}
	if (strchr((const char *)gbuffer->str,'\n') == NULL || 
		strchr((const char *)gbuffer->str,'\r') == NULL)
		return;

	printf("from client: %s\n", (char *)gbuffer->str);
	GApplication *application = g_application_new ("net.alert", G_APPLICATION_FLAGS_NONE);
	g_application_register (application, NULL, NULL);
	GNotification *notification = g_notification_new ("Net Alert");
	g_notification_set_body (notification, gbuffer->str);
	GIcon *icon = g_themed_icon_new ("dialog-information");
	g_notification_set_icon (notification, icon);
	g_application_send_notification (application, NULL, notification);
	g_object_unref (icon);
	g_object_unref (notification);
	g_object_unref (application);
	reply.base = (char *)"ack\n";
	reply.len = 4;
	uv_write(&req, stream, &reply, 1, write_status);
	g_string_erase(gbuffer, 0, -1);
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

gboolean
address_is_valid(GString *ip) {
	return g_hostname_is_ip_address(ip->str);
}

void
server_mode(){
	int res;
	uv_tcp_t server;
	struct sockaddr_in bind_addr;

	printf("server mode\n");
	uv_tcp_init(uv_default_loop(), &server);
	uv_ip4_addr("0.0.0.0", 2986, &bind_addr);
	uv_tcp_bind(&server, (const struct sockaddr *)&bind_addr, 0);
	res = uv_listen((uv_stream_t *)&server, 128, on_new_connection);
	if (res)
		return;
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	uv_loop_close(uv_default_loop());
}

void
connect_to_server(uv_connect_t *conn, int status){
	uv_buf_t msg;
	uv_write_t req;
	char *c_command;
	GString *command;
	
	printf("connected to server\n");
	command = g_string_new(config.command.str);
	g_string_append(command, "\r\n");
	if (status < 0){
		uv_shutdown_t sh;
		
		uv_shutdown(&sh, conn->handle, NULL);
		return;
	}
	msg.base = command->str;
	msg.len = command->len + 1;
	if (msg.len > 1)
		uv_write(&req, conn->handle, &msg, 1, write_status);
}

void
client_mode(){
	uv_tcp_t client;
	uv_connect_t conn;
	struct sockaddr_in bind_addr;

	printf("client mode\n");
	if (!config.server_ip.str || !address_is_valid(&config.server_ip)){
		printf("Destination address \"%s\" is not valid\n", (char *)config.server_ip.str);
		exit(EXIT_FAILURE);
	}
	if (config.server_port == 0){
		printf("Invalid port: %i\n", config.server_port);
	}
	uv_tcp_init(uv_default_loop(), &client);
	uv_ip4_addr((char *)config.server_ip.str, config.server_port, &bind_addr);
	uv_tcp_connect(&conn, &client, (const struct sockaddr *)&bind_addr, connect_to_server);
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	uv_loop_close(uv_default_loop());
}

int
main(int argc, char **argv) {
	GError *error;
	GOptionContext *context;

	error = NULL;
	config.gui = FALSE;
	config.server_mode = FALSE;
	config.server_port = 2986;
	gbuffer = g_string_new(NULL);
	context = g_option_context_new ("net_alert: an application to alert on demand");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_add_group (context, NULL);//gtk_get_option_group (TRUE));
	if (!g_option_context_parse (context, &argc, &argv, &error)){
		g_print("option parsing failed: %s\n", error->message);
		exit(EXIT_FAILURE);
	}
	printf("port: %i\nserver address: %s\nserver mode: %i\nGUI: %i\ncommand: %s\n\n"
		, config.server_port
		, (char *)config.server_ip.str
		, config.server_mode
		, config.gui
		, (char *)config.command.str);

	if (config.server_mode)
		server_mode();
	else
		client_mode();
	return 0;
}
