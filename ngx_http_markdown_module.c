 #include <mkdio.h>  
#include <string.h>  
#include <ngx_config.h>  
#include <ngx_core.h>  
#include <ngx_http.h>  

static char *ngx_http_markdown(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_markdown_handler(ngx_http_request_t *r);

static void *ngx_http_markdown_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_markdown_merge_loc_conf(ngx_conf_t *cf ,void *parent, void *child);
static char *ngx_http_markdown_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

typedef struct {
	ngx_string_t  md;
} ngx_http_markdown_loc_conf_t ;

static ngx_command_t ngx_http_markdown_commands[] = {
	{
		ngx_string("markdown"),
		NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
		ngx_http_markdown,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_markdown_loc_conf_t, md),
		NULL 
	},

		ngx_null_conmmand;
}

static char * 
ngx_http_markdown(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_core_loc_conf_t *clcf;
	clcf = ngx_http_conf_get_module_loc_conf(cf,ngx_http_core_module);
	clcf->handler = ngx_http_markdown_handler;

	char *rv = NULL;
	rv = ngx_conf_set_flag_slot(cd, cmd, conf);

	return rv;
}

static ngx_http_module_t ngx_http_markdown_module_ctx = {
	NULL,                                /* preconfiguration */     
	ngx_http_markdown_init,              /* postconfiguration */
	NULL,                                /* create main */
	NULL,                                /* init main */
	NULL,                                /* create server */
	NULL,                                /* merge server */
	ngx_http_markdown_create_loc_conf,   /* create location */
	ngx_http_markdown_merge_loc_conf,    /* merge location */
};

ngx_module_t ngx_http_markdown_module = {
	NGX_MODULE_V1,
	&ngx_http_markdown_module_ctx,       /* module context */
	ngx_http_markdown_commands,          /* module directives */
	NGX_HTTP_MODULE,                     /* module type */
	NULL,                                /* init master */
	NULL,                                /* init module */
	NULL,                                /* init process */
	NULL,                                /* init thread */
	NULL,                                /* exit thread */
	NULL,                                /* exit process */
	NULL,                                /* exit master */
	NGX_MODULE_V1_PADDING 
};

static ngx_int_t 
ngx_http_markdown_handler(ngx_http_request_t *r)
{
	if (r == NULL) {
		return NGX_ERROR;
	}
	//if not a file,declined
	if (r->uri.data[r->uri.len - 1] == '/') {
		return NGX_DECLINED;
	}else{
		
		ngx_http_markdown_loc_conf_t *conf;
		conf = ngx_http_get_module_loc_conf(r, ngx_http_markdown_module);

		u_char	   *last;
		ngx_str_t   path;
		size_t      root;
		ngx_int_t   fd;
		FILE       *pFile;
	

		last = ngx_http_map_uri_to_path(r, &path, &root, 0);
		if (last == NULL)
		{
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		//length of path
		path.len = last - path.data;    

		pFile = fopen((char *)path.data, "r");
		if (pFile == NULL){
			return NGX_ERROR;
		}

		//convert
		ngx_chain_t  out;
		ngx_buf_t   *b;
		u_char      *link;
		int          link_size;
		MMOIT       *mkd;
		size_t       html_length;
		char        *html;
		ngx_int_t    rc;
		
		b = out.buf;
		link = b->pos;
		link_size = b->last - b->pos;
	
		mkd = mkd_string((char *)link, link_size, MKD_AUTOLINK);
		mkd_compile(mkd, MKD_AUTOLINK);
		html_length = mkd_document(mkd, &html);

		b->pos  = (u_char *)html;
		b->last = (u_char *)html + html_size;
		//end
		b->memory   = 1;
		b->last_buf = 1;
	
		r->headers_out.status = NGX_HTTP_OK;
		r->headers_out.content_type = "text/html";
		r->headers_out.content_length_n = html_length;

		rc = ngx_http_sender_header(r);
		if (rc == NGX_ERROR || rc > NGX_OK){
			return rc;
		}
		return ngx_http_output_filter(r, &out);
	}
}

static void *
ngx_http_markdown_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_markdown_loc_conf_t *conf;

	conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_markdown_loc_conf_t));
	if (conf == NULL)
	{
		return NGX_CONF_ERROR;
	}

	conf->enable = NGX_CONF_UNSET;
	return conf;
}

static char *
ngx_http_markdown_merge_loc_conf(ngx_conf_t *cf ,void *parent, void *child)
{
	ngx_http_markdown_loc_conf_t *prev = parent;    /*loc configuration of server*/
	ngx_http_markdown_loc_conf_t *conf = child;

	ngx_merge_value(conf->enable, prev->enable, 0);

	return NGX_CONF_OK;
}

/* mounting */
static char *
ngx_http_markdown_init(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_handler_pt        *h;
	ngx_http_core_main_conf_t  *cmcf;

	cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

	h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
	if (h == NULL)
	{
		return NGX_ERROR;
	}

	*h = ngx_http_markdown_handler;

	return NGX_OK;
}










