#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

#include <stdio.h>
#include <ctype.h>

#ifdef WIN32
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32")
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#if !defined (WIN32)
#include <dlfcn.h>
#define GetProcAddress dlsym
#define GetLastError dlerror

#if defined (SHARELIB)
#define HIGHERTHAN8
char jt_signature[64] = {'8'};
#endif
void* hMod;
#else
HMODULE hMod;
#endif

int use_ngx_plugin = 0;

typedef void (*frame_init_pt)();
typedef void (*frame_release_pt)();
typedef void (*set_set_out_header_pt)(void *set_out_header_pt);
typedef void (*set_register_connection_cleanup_pt)(void *register_connection_cleanup_pt);
typedef void (*set_register_request_cleanup_pt)(void *register_request_cleanup_pt);
typedef void (*set_ngx_http_write_back_pt)(void *ngx_http_write_back_pt);
typedef void (*set_replace_out_body_pt)(void *replace_out_body_pt);
typedef void (*set_http_finalize_request_pt)(void *http_finalize_request_pt);
typedef void (*set_get_connection_map_pt)(void *get_connection_map_pt);
typedef void (*set_get_request_map_pt)(void *get_request_map_pt);
typedef void (*set_get_request_or_response_data_pt)(void *get_request_or_response_data_pt);
typedef void (*store_data_by_type_pt)(const char *key, size_t n_key, void *value, size_t n_value, void *data, int type);
typedef void (*init_map_pt)(void *data);
typedef void (*connection_cleanup_handle_pt)(void *data);
typedef int (*connection_check_pt)(void *data);
typedef void (*ngx_request_cleanup_handle_pt)(void *data);
typedef int (*request_check_pt)(void *data, uintptr_t method);
typedef void (*response_header_check_pt)(void *data);
typedef void (*response_body_check_pt)(void *data, void **body);
typedef int (*post_in_check_pt)(void *data, void *post_buf);
typedef int (*post_in_check2_pt)(void *data, void *post_buf);

frame_init_pt frame_init;
frame_release_pt frame_release;
set_set_out_header_pt set_set_out_header_handler;
set_register_connection_cleanup_pt set_register_connection_cleanup_handler;
set_register_request_cleanup_pt set_register_request_cleanup_handler;
set_ngx_http_write_back_pt set_ngx_http_write_back_handler;
set_replace_out_body_pt set_replace_out_body_handler;
set_http_finalize_request_pt set_http_finalize_request_handler;
set_get_connection_map_pt set_get_connection_map_handler;
set_get_request_map_pt set_get_request_map_handler;
set_get_request_or_response_data_pt set_get_request_or_response_data_handler;
store_data_by_type_pt store_data_by_type;
init_map_pt init_map;
connection_cleanup_handle_pt connection_cleanup_handle;
connection_check_pt connection_check;
ngx_request_cleanup_handle_pt ngx_request_cleanup_handle;
request_check_pt request_check;
response_header_check_pt response_header_check;
response_body_check_pt response_body_check;
post_in_check_pt post_in_check;
post_in_check2_pt post_in_check2;

#if !defined (WIN32)
void get_yunsuo_instatll_path(char *install_path, int *path_len)
{
	int len = 0;
	int flags = 0;
	char buf[64] = { 0 };
	char *p_enter_pos = NULL;

	FILE *fp = fopen("/var/log/version_control", "r");
	
	do 
	{
		if (!fp)
		{
			break;
		}
		
		if(!fgets(buf, 64, fp))
		{
			break;
		}

		len = strlen(buf);
		if (len > *path_len)
		{
			fclose(fp);
			break;
		}
		
		if ((p_enter_pos = strstr(buf, "\n")) != NULL)
		{
			*p_enter_pos = '\0';
			len = strlen(buf);
		}
		
		memcpy(install_path, buf, len);
		*path_len = len;

		fclose(fp);
		flags = 1;
	} while (0);
	
	if (!flags)
	{
		memcpy(install_path, "/usr/local/yunsuo_agent", 23);
		*path_len = 23;
	}
}
#endif

int init_functions(ngx_cycle_t *cycle)
{
	int result = 0;
	int len = 128;
	char ngx_plugin_path[128] = {0};
	
#if defined (WIN32)
	hMod = LoadLibrary(ngx_plugin_path);
#else
	get_yunsuo_instatll_path(ngx_plugin_path, &len);
	sprintf((char*)ngx_plugin_path + len, "%s", "/nginx/libnginx_plugin.so");
	hMod = dlopen(ngx_plugin_path, RTLD_NOW);
#endif
	if (hMod)
	{
		frame_init = GetProcAddress(hMod, "frame_init");
		frame_release = GetProcAddress(hMod, "frame_release");
		set_set_out_header_handler = GetProcAddress(hMod, "set_set_out_header_handler");
		set_register_connection_cleanup_handler = GetProcAddress(hMod, "set_register_connection_cleanup_handler");
		set_register_request_cleanup_handler = GetProcAddress(hMod, "set_register_request_cleanup_handler");
		set_ngx_http_write_back_handler = GetProcAddress(hMod, "set_ngx_http_write_back_handler");
		set_replace_out_body_handler = GetProcAddress(hMod, "set_replace_out_body_handler");
		set_http_finalize_request_handler = GetProcAddress(hMod, "set_http_finalize_request_handler");
		set_get_connection_map_handler = GetProcAddress(hMod, "set_get_connection_map_handler");
		set_get_request_map_handler = GetProcAddress(hMod, "set_get_request_map_handler");
		set_get_request_or_response_data_handler = GetProcAddress(hMod, "set_get_request_or_response_data_handler");
		store_data_by_type = GetProcAddress(hMod, "store_data_by_type");
		init_map = GetProcAddress(hMod, "init_map");
		connection_cleanup_handle = GetProcAddress(hMod, "connection_cleanup_handle");
		connection_check = GetProcAddress(hMod, "connection_check");
		ngx_request_cleanup_handle = GetProcAddress(hMod, "ngx_request_cleanup_handle");
		request_check = GetProcAddress(hMod, "request_check");
		response_header_check = GetProcAddress(hMod, "response_header_check");
		response_body_check = GetProcAddress(hMod, "response_body_check");
		post_in_check = GetProcAddress(hMod, "post_in_check");		
		post_in_check2 = GetProcAddress(hMod, "post_in_check2");
	}
	else
	{
		ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "%s", GetLastError()); 
		result = -1;
	}

	return result;
}

static void dll_release()
{
	if (!hMod)
	{
		return;
	}	
#if defined (WIN32)
	FreeLibrary(hMod);
#else
	dlclose(hMod);
#endif
}


ngx_http_output_header_filter_pt  ngx_http_top_header_filter;
ngx_http_output_body_filter_pt    ngx_http_top_body_filter;
static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt ngx_http_next_body_filter;

#if defined(TENGINE) 
#if !defined (HIGHERTHAN8)
ngx_http_input_body_filter_pt     ngx_http_top_input_body_filter;
static ngx_http_input_body_filter_pt  ngx_http_next_input_body_filter;
ngx_int_t tengine_http_yunsuo_post_body_filter(ngx_http_request_t *r, ngx_buf_t *buf)
{
	if (post_in_check(r, buf))
	{
		return NGX_OK;
	}
	return ngx_http_next_input_body_filter(r, buf);
}
#endif
#endif

#if defined (HIGHERTHAN8)
ngx_http_request_body_filter_pt   ngx_http_top_request_body_filter;
static ngx_http_request_body_filter_pt	ngx_http_next_request_body_filter;
ngx_int_t http_yunsuo_post_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
	if (post_in_check2 != NULL && post_in_check2(r, in))
	{
		return NGX_OK;
	}
	return ngx_http_next_request_body_filter(r, in);
}
#endif

static ngx_int_t ngx_http_yunsuo_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_yunsuo_body_filter(ngx_http_request_t *r, ngx_chain_t *in);
static ngx_int_t ngx_http_yunsuo_handler(ngx_http_request_t *r);

typedef struct
{
	void *pMap;
	void *request;
	long out_body_len;
}ngx_cleanup_callback_dt;

static void ngx_connection_cleanup_handler(void *data)
{
	connection_cleanup_handle(data);
}

static int register_connection_cleanup_handler(void *request)
{
	ngx_cleanup_callback_dt *data;
	ngx_http_request_t *r = (ngx_http_request_t *)request;
	int flags = 0;
	ngx_pool_cleanup_t *pcln;
	ngx_pool_cleanup_t *c_cleanup;
	for (c_cleanup = r->connection->pool->cleanup; c_cleanup; c_cleanup = c_cleanup->next) 
	{
		if (c_cleanup->handler && c_cleanup->handler == ngx_connection_cleanup_handler) 
		{         
			flags = 1;
			break;
		}
	}
	if (flags)
	{
		return 0;
	}

	pcln = ngx_pool_cleanup_add(r->connection->pool, sizeof(ngx_cleanup_callback_dt));
	if(!pcln)
	{
		return -1;
	}
	pcln->handler = ngx_connection_cleanup_handler;
	data = (ngx_cleanup_callback_dt*)pcln->data;
	init_map(data);

	return 1;
}

static void ngx_request_cleanup_handler(void *data)
{
	ngx_request_cleanup_handle(data);
}

static int register_request_cleanup_handler(void *request)
{
	ngx_cleanup_callback_dt *p_cleanup_data;
	ngx_http_request_t *r = (ngx_http_request_t *)request;	
	ngx_http_cleanup_t *cln = ngx_http_cleanup_add(r, sizeof(ngx_cleanup_callback_dt));
	if (NULL == cln) {

		return -1;
	}

	cln->handler = ngx_request_cleanup_handler;
	p_cleanup_data = (ngx_cleanup_callback_dt *)cln->data;
	p_cleanup_data->request = request;
	p_cleanup_data->out_body_len = 0;

	init_map(p_cleanup_data);

	return 0;
}

static void set_out_header(void *request, const char* name, const char* val, int len)
{
	ngx_http_request_t *r = (ngx_http_request_t *)request;
	if (0 != len)
	{
		r->headers_out.content_type.data = (u_char*)"text/html";
		r->headers_out.content_type.len = sizeof("text/html")-1;
		r->headers_out.status = NGX_HTTP_OK;
		r->headers_out.content_length_n = len;
		ngx_str_set(&r->headers_out.status_line, "200 OK");
	}
	else
	{
		int n_value = 0;
		int n_key = 0;
		u_char *data = NULL;
		
		ngx_table_elt_t *header = (ngx_table_elt_t*)ngx_list_push(&r->headers_out.headers);
		if (NULL == header) {
			return;
		}

		n_value = strlen(val);
		n_key = strlen(name);
		data = (u_char*)ngx_pnalloc(r->pool, n_value + n_key);
		if (NULL == data) {
			return;
		}
		ngx_memcpy(data, name, n_key);
		ngx_memcpy(data + n_key, val, n_value);

		header->hash = 1;
		header->key.data = data;
		header->key.len = n_key;

		header->value.len = n_value;
		header->value.data = data + n_key;
	}
}

int ngx_http_write_back(void *request, const char* content_type, const char* html)
{
	ngx_chain_t out;
	int content_type_len;
	int html_len;
	ngx_int_t rc = NGX_OK;
	ngx_buf_t* b = NULL;
	ngx_http_request_t *r = (ngx_http_request_t *)request;

	if (NULL == html)
	{
		return rc;
	}
	if (0 == (html_len = strlen(html)))
	{
		return rc;
	}

	content_type_len = strlen(content_type);
	if (content_type_len == 0)
	{
		return rc;
	}

	r->headers_out.content_type.len = content_type_len;
	r->headers_out.content_type.data = (u_char*)content_type;
	r->headers_out.status = NGX_HTTP_OK;

	b = ngx_create_temp_buf(r->pool, html_len);
	if(!b) 
	{
		return rc; 
	}

	b->last = (u_char*)ngx_cpymem(b->pos, html, html_len);
	b->memory = 1;
	b->last_buf = 1;

	r->headers_out.content_length_n = html_len;
	rc = ngx_http_send_header(r);

	out.buf = b;
	out.next = NULL;

	return ngx_http_output_filter(r, &out);
}

void replace_out_body(void *request, const char *new_body, void **old_body)
{
	ngx_chain_t **pIn = (ngx_chain_t **)old_body;
	ngx_chain_t *in = *pIn;
	int len = strlen(new_body);
	ngx_http_request_t *r = (ngx_http_request_t *)request;

	ngx_buf_t* b = ngx_create_temp_buf(r->pool, len);
	if(!b) 
	{
		return; 
	}

	if (!in)
	{
		in = (ngx_chain_t*)ngx_pnalloc(r->pool, sizeof(ngx_chain_t));
	}

	b->last = (u_char*)ngx_cpymem(b->pos, new_body, len);
	b->memory = 1;
	b->last_buf = 1;

	in->buf = b;
	in->next = NULL;
}

void http_finalize_request_handler(void *request)
{
	ngx_http_request_t *r = (ngx_http_request_t *)request;
	ngx_http_finalize_request(r, NGX_HTTP_OK);
}

void *get_connection_map_handler(void *request)
{
	void *result = NULL;
	ngx_cleanup_callback_dt *p_cleanup_data;
	ngx_http_request_t *r = (ngx_http_request_t *)request;
	ngx_pool_cleanup_t *c_cleanup;
	for (c_cleanup = r->connection->pool->cleanup; c_cleanup; c_cleanup = c_cleanup->next) 
	{
		if (c_cleanup->handler && c_cleanup->handler == ngx_connection_cleanup_handler) 
		{         
			p_cleanup_data = (ngx_cleanup_callback_dt *)c_cleanup->data;
			result = p_cleanup_data->pMap;
			break;
		}
	}

	return result;
}

void *get_request_map_handler(void *request)
{
	void *result = NULL;
	ngx_cleanup_callback_dt *p_cleanup_data;
	ngx_http_request_t *r = (ngx_http_request_t *)request;
	ngx_http_cleanup_t *c_cleanup;
	for (c_cleanup = r->cleanup; c_cleanup; c_cleanup = c_cleanup->next) 
	{
		if (c_cleanup->handler && c_cleanup->handler == ngx_request_cleanup_handler) 
		{         
			p_cleanup_data = (ngx_cleanup_callback_dt *)c_cleanup->data;
			result = p_cleanup_data;
			break;
		}
	}

	return result;
}

static int check_capital(const char *src, size_t src_len, char *dst, size_t dst_len) 
{
	int i;
	int next_pos = 0;
	int len = src_len <= dst_len ? src_len : dst_len;

	if (isupper(src[0]))
	{
		return 1;
	}

	for (i = 0; i < len; i++)
	{
		if (i == next_pos)
		{
			dst[i] = toupper(src[i]);
			continue;
		}

		if (src[i] == '-')
		{
			next_pos = i + 1;
		}

		dst[i] = src[i];
	}
	return 0;
}

static void traverse_header_fields(void *request, int is_in_header, void *data)
{
	unsigned int i;
	ngx_http_request_t *r = (ngx_http_request_t *)request;
	ngx_list_part_t* part;
	ngx_table_elt_t* header;
	if (is_in_header)
	{
		part = &r->headers_in.headers.part;
	}
	else
	{
		part = &r->headers_out.headers.part;
	}
	
	header = (ngx_table_elt_t*)part->elts;

	for (i = 0;; i++)
	{
		if (i >= part->nelts)
		{
			if (NULL == part->next)
			{
				break;
			}
			part = part->next;
			header = (ngx_table_elt_t*)part->elts;
			i = 0;
		}

		if (header[i].hash == 0)
		{
			continue;
		}

		if (header[i].key.data && header[i].value.data)
		{
			char buf[128] = {0};
			int result = check_capital((char*)header[i].key.data, header[i].key.len, buf, 127);
			if (result)
			{
			store_data_by_type((char*)header[i].key.data, header[i].key.len, (char*)header[i].value.data, header[i].value.len, data, 0);
		}
			else 
			{
				store_data_by_type(buf, strlen(buf), (char*)header[i].value.data, header[i].value.len, data, 0);
			}	
		}
	}
}

void get_request_or_response_data_handler(void *request, void *data, int data_type)
{
	ngx_http_request_t *r = (ngx_http_request_t *)request;
	if (0 == data_type)
	{
		if (r->connection && r->connection->addr_text.data)
		{
			store_data_by_type("Remote-Ip", 9, (char*)r->connection->addr_text.data, r->connection->addr_text.len, data, 0);
		}

		if (r->connection && r->connection->listening && r->connection->listening->addr_text.data)
		{
			ngx_str_t local_addr = r->connection->listening->addr_text;
			store_data_by_type("Local-Ip", 8, local_addr.data, local_addr.len, data, 0);
		}

		if (r->headers_in.host && r->headers_in.host->value.data )
		{
			store_data_by_type("Local-Host", 10, (char*)r->headers_in.host->value.data, r->headers_in.host->value.len, data, 0);
		}
	}
	else if (1 == data_type)
	{
		struct sockaddr_in *sin;
		size_t path_len = 0;
		ngx_str_t full_path;

		if (NULL != ngx_http_map_uri_to_path(r, &full_path, &path_len, 0))
		{
			size_t length = full_path.len;
			if (full_path.data[length-1] == '\0')
			{
				length = strlen((char*)full_path.data);
			}
			store_data_by_type("File-Path", 9, (char*)full_path.data, length, data, 0);
		}
		if (r->uri.data)
		{
			store_data_by_type("Url", 3, (char*)r->uri.data, r->uri.len, data, 0);
		}
		if (r->args.data)
		{
			store_data_by_type("Query", 5, (char*)r->args.data, r->args.len, data, 0);
		}
		if (r->method_name.data)
		{
			store_data_by_type("Method", 6, (char*)r->method_name.data, r->method_name.len, data, 0);
		}

		if (r->connection && r->connection->listening && r->connection->listening->sockaddr)
		{
			sin = (struct sockaddr_in *)r->connection->listening->sockaddr;
		if (NULL != sin)
		{
			int len = 0;
			char buf[32] = {0};
			int port = ntohs(sin->sin_port);
#ifdef WIN32
			len = sprintf_s(buf, 32, "%d", port);		
#else
			len = snprintf(buf, 32, "%d", port);
#endif
			store_data_by_type("Port", 4, buf, len, data, 0);
		}
		}
		
		traverse_header_fields(r, 1, data);
	}
	else if (2 == data_type)
	{
		size_t len = 0;
		char buf[4] = {0};
		ngx_http_request_t *r = (ngx_http_request_t *)request;
#ifdef WIN32
		len = sprintf_s(buf, 4, "%d", r->headers_out.status);		
#else
		len = snprintf(buf, 4, "%d", (int)r->headers_out.status);
#endif
		store_data_by_type("Status", 6, buf, len, data, 0);
		traverse_header_fields(r, 0, data);
	}
	else if (3 == data_type)
	{
		store_data_by_type("Header-Size", 11, NULL, r->header_size, data, 1);
		store_data_by_type("Request-Length", 14, NULL, (size_t)r->request_length, data, 1);
	}
	else if (4 == data_type)
	{
		if (r->headers_in.host && r->headers_in.host->value.data )
		{
			store_data_by_type("Host", 4, (char*)r->headers_in.host->value.data, r->headers_in.host->value.len, data, 0);
		}
		if (r->headers_out.content_type.data)
		{
			store_data_by_type("Content-Type", 12, (char*)r->headers_out.content_type.data, r->headers_out.content_type.len, data, 0);
		}
		if (r->headers_out.content_encoding && r->headers_out.content_encoding->value.data)
		{
			store_data_by_type("Content-Encoding", 16, (char*)r->headers_out.content_encoding->value.data, r->headers_out.content_encoding->value.len, data, 0);
		}
	}
	else if(5 == data_type)
	{
		if (r->request_body && r->request_body->bufs)
		{
			store_data_by_type("Post-Bufs", 9, (void*)r->request_body->bufs, 0, data, 2);
		}
	}
	else {
		if (r->headers_in.content_type && r->headers_in.content_type->value.data)
		{
			store_data_by_type("Content-Type_IN", 15, (char*)r->headers_in.content_type->value.data, r->headers_in.content_type->value.len, data, 0);
		}
	}
}

static ngx_int_t yunsuo_init_process(ngx_cycle_t *cycle)
{
	if (0 != init_functions(cycle))
	{
		return NGX_OK;
	}
	use_ngx_plugin = 1;

	frame_init();

	set_set_out_header_handler(set_out_header);
	set_register_connection_cleanup_handler(register_connection_cleanup_handler);
	set_register_request_cleanup_handler(register_request_cleanup_handler);
	set_ngx_http_write_back_handler(ngx_http_write_back);
	set_replace_out_body_handler(replace_out_body);
	set_http_finalize_request_handler(http_finalize_request_handler);

	set_get_connection_map_handler(get_connection_map_handler);
	set_get_request_map_handler(get_request_map_handler);
	
	set_get_request_or_response_data_handler(get_request_or_response_data_handler);

	return NGX_OK;
}

static void yunsuo_exit_process(ngx_cycle_t *cycle)
{
	if (0 == use_ngx_plugin)
	{
		return;
	}
	frame_release();
	dll_release();
}

static ngx_int_t ngx_http_yunsuo_filter_init(ngx_conf_t *cf)
{
	ngx_http_handler_pt        *h;
	ngx_http_core_main_conf_t  *cmcf;

	cmcf = (ngx_http_core_main_conf_t *)ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

	h = (ngx_http_handler_pt *)ngx_array_push(&cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers);
	if (h == NULL) 
	{
		return NGX_ERROR;
	}

	*h = ngx_http_yunsuo_handler;

	ngx_http_next_header_filter = ngx_http_top_header_filter;
	ngx_http_top_header_filter = ngx_http_yunsuo_header_filter;

	ngx_http_next_body_filter = ngx_http_top_body_filter;
	ngx_http_top_body_filter = ngx_http_yunsuo_body_filter;

#if defined(TENGINE)
#if !defined (HIGHERTHAN8)
	ngx_http_next_input_body_filter = ngx_http_top_input_body_filter;
	ngx_http_top_input_body_filter = tengine_http_yunsuo_post_body_filter;
#endif
#endif

#if defined (HIGHERTHAN8)
	ngx_http_next_request_body_filter = ngx_http_top_request_body_filter;
	ngx_http_top_request_body_filter = http_yunsuo_post_body_filter;
#endif

	return NGX_OK;
}

static ngx_command_t  ngx_http_yunsuo_commands[] = 
{
	ngx_null_command
};

static ngx_http_module_t  ngx_http_yunsuo_module_ctx = 
{
	NULL,                                  /* preconfiguration */
	ngx_http_yunsuo_filter_init,          /* postconfiguration */
	NULL,  /* create main configuration */
	NULL,                                  /* init main configuration */
	NULL,                                  /* create server configuration */
	NULL,                                  /* merge server configuration */
	NULL,          /* create location configuration */
	NULL	/* merge location configuration */
};


ngx_module_t  ngx_http_yunsuo_module = {
#if defined (SHARELIB)
	(ngx_uint_t) -1, (ngx_uint_t) -1,                           \
	NULL, 0, 0, nginx_version, jt_signature,
#else
	NGX_MODULE_V1,
#endif
	&ngx_http_yunsuo_module_ctx,   /* module context */
	ngx_http_yunsuo_commands,      /* module directives */
	NGX_HTTP_MODULE,                       /* module type */
	NULL,                                  /* init master */
	NULL,                                  /* init module */
	yunsuo_init_process,            /* init process */
	NULL,                                  /* init thread */
	NULL,                                  /* exit thread */
	yunsuo_exit_process,            /* exit process */
	NULL,                                  /* exit master */
	NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_yunsuo_header_filter(ngx_http_request_t *r)
{
	if (0 == use_ngx_plugin)
	{
		return ngx_http_next_header_filter(r);
	}	
	response_header_check(r);
	return ngx_http_next_header_filter(r);	
}

static ngx_int_t ngx_http_yunsuo_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
	ngx_chain_t **dpIn = &in;

	if (0 == use_ngx_plugin)
	{
		return ngx_http_next_body_filter(r, *dpIn);
	}	

	response_body_check(r, (void**)dpIn);
	return ngx_http_next_body_filter(r, *dpIn);
}

int ngx_http_yunsuo_post_in_handler(ngx_http_request_t *r) 
{ 
	if ((0 == (r->method & NGX_HTTP_POST)) || (NULL == r->request_body))
	{
		return 0;
	}

	if (0 == use_ngx_plugin)
	{
		return 0;
	}	

	return post_in_check(r, NULL);
}

static ngx_int_t ngx_http_yunsuo_handler(ngx_http_request_t *r)
{
	int result = 0;

	if (0 == use_ngx_plugin)
	{
		return NGX_DECLINED;
	}	

	result = connection_check(r);
	if (result == -2)
	{
		return NGX_DONE;
	}
	else if(result == -1)
	{
		return NGX_DECLINED; 
	}

	return request_check(r, r->method);       
}

