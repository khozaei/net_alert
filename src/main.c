#include <uv.h>
#include <stdio.h>
#include <stdlib.h>

void write_status(uv_write_t *, int status){
	if (status == 0)
		printf("replied successfully!\n");
}

void buffer_reader(uv_stream_t *stream, ssize_t nbytes, const uv_buf_t *buf) {
	uv_buf_t reply;
	uv_write_t req;

	if (nbytes < 0)
		uv_read_stop(stream);
	printf("from client: %s\n", buf->base);
	reply.base = (char *)"reply from tcpserver\n";
	reply.len = 21;
	uv_write(&req, stream, &reply, 1, write_status);
}

void buffer_allocator(uv_handle_t *, size_t sug_size, uv_buf_t *buf) {
	buf->base = malloc(sug_size);
	buf->len = sug_size;
}

void on_new_connection(uv_stream_t *server, int status) {
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
main() {
	uv_tcp_t server;
	struct sockaddr_in bind_addr;
	int res;

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
