#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <stdio.h>

typedef struct {
	ngx_flag_t        enabled;
	ngx_array_t      *messages;   /* of ngx_str_t */
	ngx_str_t         message_file;
} ngx_http_huyangix_loc_conf_t;

typedef struct {
	unsigned          active:1;
	unsigned          sent:1;
	ngx_str_t         body;
} ngx_http_huyangix_ctx_t;

static ngx_int_t ngx_http_huyangix_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_huyangix_body_filter(ngx_http_request_t *r, ngx_chain_t *in);
static ngx_int_t ngx_http_huyangix_filter_init(ngx_conf_t *cf);

static void *ngx_http_huyangix_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_huyangix_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static char *ngx_http_huyangix_set_message_file(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

static ngx_str_t ngx_http_huyangix_default_messages[] = {
	ngx_string("Хуй тебе, а не страница!"),
	ngx_string("Oops! Где-то ты свернул не туда."),
	ngx_string("Сервер устал. Иди попей чай."),
};

static ngx_command_t ngx_http_huyangix_commands[] = {

	{ ngx_string("huyanginx"),
	  NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
	  ngx_conf_set_flag_slot,
	  NGX_HTTP_LOC_CONF_OFFSET,
	  offsetof(ngx_http_huyangix_loc_conf_t, enabled),
	  NULL },

	{ ngx_string("huyangix_message_file"),
	  NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
	  ngx_http_huyangix_set_message_file,
	  NGX_HTTP_LOC_CONF_OFFSET,
	  offsetof(ngx_http_huyangix_loc_conf_t, message_file),
	  NULL },

	ngx_null_command
};

static ngx_http_module_t ngx_http_huyangix_module_ctx = {
	NULL,                           /* preconfiguration */
	ngx_http_huyangix_filter_init,  /* postconfiguration */

	NULL,                           /* create main configuration */
	NULL,                           /* init main configuration */

	NULL,                           /* create server configuration */
	NULL,                           /* merge server configuration */

	ngx_http_huyangix_create_loc_conf, /* create location configuration */
	ngx_http_huyangix_merge_loc_conf   /* merge location configuration */
};

ngx_module_t ngx_http_huyangix_module = {
	NGX_MODULE_V1,
	&ngx_http_huyangix_module_ctx,   /* module context */
	ngx_http_huyangix_commands,      /* module directives */
	NGX_HTTP_MODULE,                 /* module type */
	NULL,                            /* init master */
	NULL,                            /* init module */
	NULL,                            /* init process */
	NULL,                            /* init thread */
	NULL,                            /* exit thread */
	NULL,                            /* exit process */
	NULL,                            /* exit master */
	NGX_MODULE_V1_PADDING
};

static void *
ngx_http_huyangix_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_huyangix_loc_conf_t  *conf;

	conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_huyangix_loc_conf_t));
	if (conf == NULL) {
		return NULL;
	}

	conf->enabled = NGX_CONF_UNSET;
	conf->messages = NULL;
	conf->message_file.len = 0;
	conf->message_file.data = NULL;

	return conf;
}

static char *
ngx_http_huyangix_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
	ngx_http_huyangix_loc_conf_t *prev = parent;
	ngx_http_huyangix_loc_conf_t *conf = child;

	ngx_conf_merge_value(conf->enabled, prev->enabled, 0);

	if (conf->messages == NULL) {
		conf->messages = prev->messages;
	}

	if (conf->message_file.data == NULL) {
		conf->message_file = prev->message_file;
	}

	return NGX_CONF_OK;
}

static char *
ngx_http_huyangix_set_message_file(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_huyangix_loc_conf_t *hlcf = conf;
	ngx_str_t                    *value;
	ngx_str_t                     path;

	if (hlcf->message_file.data != NULL) {
		return "is duplicate";
	}

	value = cf->args->elts;
	hlcf->message_file = value[1];
	path = hlcf->message_file;

	if (ngx_conf_full_name(cf->cycle, &path, 0) != NGX_OK) {
		return NGX_CONF_ERROR;
	}

	FILE *fp = fopen((const char *) path.data, "r");
	if (fp == NULL) {
		ngx_conf_log_error(NGX_LOG_ERR, cf, ngx_errno,
			"huyangix: failed to open message file \"%V\"", &path);
		return NGX_CONF_ERROR;
	}

	hlcf->messages = ngx_array_create(cf->pool, 8, sizeof(ngx_str_t));
	if (hlcf->messages == NULL) {
		fclose(fp);
		return NGX_CONF_ERROR;
	}

	char   *line = NULL;
	size_t  cap = 0;
	ssize_t nread;
	while ((nread = getline(&line, &cap, fp)) != -1) {
		/* trim trailing CR/LF */
		while (nread > 0 && (line[nread - 1] == '\n' || line[nread - 1] == '\r')) {
			nread--;
		}
		/* skip leading spaces/tabs */
		size_t start = 0;
		while (start < (size_t) nread && (line[start] == ' ' || line[start] == '\t')) {
			start++;
		}
		if ((size_t) nread <= start) {
			continue;
		}
		if (line[start] == '#') {
			continue;
		}

		ngx_str_t *msg = ngx_array_push(hlcf->messages);
		if (msg == NULL) {
			free(line);
			fclose(fp);
			return NGX_CONF_ERROR;
		}
		msg->len = (size_t) nread - start;
		msg->data = ngx_pnalloc(cf->pool, msg->len);
		if (msg->data == NULL) {
			free(line);
			fclose(fp);
			return NGX_CONF_ERROR;
		}
		ngx_memcpy(msg->data, (u_char *) (line + start), msg->len);
	}
	if (line) {
		free(line);
	}
	fclose(fp);

	return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_huyangix_header_filter(ngx_http_request_t *r)
{
	ngx_http_huyangix_loc_conf_t *hlcf;
	ngx_http_huyangix_ctx_t      *ctx;

	hlcf = ngx_http_get_module_loc_conf(r, ngx_http_huyangix_module);
	if (hlcf == NULL || !hlcf->enabled) {
		return ngx_http_next_header_filter(r);
	}

	if (r->headers_out.status != NGX_HTTP_NOT_FOUND) {
		return ngx_http_next_header_filter(r);
	}

	ctx = ngx_http_get_module_ctx(r, ngx_http_huyangix_module);
	if (ctx == NULL) {
		ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_huyangix_ctx_t));
		if (ctx == NULL) {
			return NGX_ERROR;
		}
		ngx_http_set_ctx(r, ctx, ngx_http_huyangix_module);
	}

	/* choose a message */
	ngx_str_t msg;
	ngx_uint_t nmsgs = 0;
	if (hlcf->messages && hlcf->messages->nelts > 0) {
		nmsgs = hlcf->messages->nelts;
		ngx_str_t *elts = hlcf->messages->elts;
		msg = elts[ngx_random() % nmsgs];
	} else {
		nmsgs = sizeof(ngx_http_huyangix_default_messages)/sizeof(ngx_http_huyangix_default_messages[0]);
		msg = ngx_http_huyangix_default_messages[ngx_random() % nmsgs];
	}

	/* build body: "404 " + msg */
	ctx->body.len = sizeof("404 ") - 1 + msg.len;
	ctx->body.data = ngx_pnalloc(r->pool, ctx->body.len);
	if (ctx->body.data == NULL) {
		return NGX_ERROR;
	}
	ngx_memcpy(ctx->body.data, (u_char *)"404 ", sizeof("404 ") - 1);
	ngx_memcpy(ctx->body.data + (sizeof("404 ") - 1), msg.data, msg.len);

	ctx->active = 1;
	ctx->sent = 0;

	/* set headers for our plain text body */
	ngx_str_t type = ngx_string("text/plain; charset=utf-8");
	r->headers_out.content_type = type;
	r->headers_out.content_type_len = type.len;
	r->headers_out.content_type_lowcase = NULL;
	r->headers_out.content_length_n = (off_t) ctx->body.len;

	ngx_http_clear_etag(r);
	ngx_http_clear_last_modified(r);
	ngx_http_clear_accept_ranges(r);

	return ngx_http_next_header_filter(r);
}

static ngx_int_t
ngx_http_huyangix_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
	ngx_http_huyangix_ctx_t *ctx;

	ctx = ngx_http_get_module_ctx(r, ngx_http_huyangix_module);
	if (ctx == NULL || !ctx->active) {
		return ngx_http_next_body_filter(r, in);
	}

	if (!ctx->sent) {
		ngx_buf_t   *b;
		ngx_chain_t  out;

		b = ngx_calloc_buf(r->pool);
		if (b == NULL) {
			return NGX_ERROR;
		}
		b->pos = ctx->body.data;
		b->last = ctx->body.data + ctx->body.len;
		b->memory = 1;
		b->last_buf = 1;
		b->last_in_chain = 1;

		out.buf = b;
		out.next = NULL;

		ctx->sent = 1;
		/* send our body instead of the original */
		return ngx_http_next_body_filter(r, &out);
	}

	/* swallow any further body */
	return NGX_OK;
}

static ngx_int_t
ngx_http_huyangix_filter_init(ngx_conf_t *cf)
{
	ngx_http_next_header_filter = ngx_http_top_header_filter;
	ngx_http_top_header_filter = ngx_http_huyangix_header_filter;

	ngx_http_next_body_filter = ngx_http_top_body_filter;
	ngx_http_top_body_filter = ngx_http_huyangix_body_filter;

	return NGX_OK;
}


