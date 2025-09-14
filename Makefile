SHELL := /bin/bash
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_DIR := $(ROOT)/build
NGINX_VERSION ?= $(shell nginx -v 2>&1 | awk -F/ '{print $$2}')
NGINX_SRC := $(BUILD_DIR)/nginx-$(NGINX_VERSION)
SO_PATH := $(NGINX_SRC)/objs/ngx_http_huyangix_module.so

.PHONY: all build clean test

all: build

$(SO_PATH):
	@mkdir -p $(BUILD_DIR)
	@echo "==> Downloading nginx-$(NGINX_VERSION) source..."
	@curl -fsSL -o $(BUILD_DIR)/nginx-$(NGINX_VERSION).tar.gz https://nginx.org/download/nginx-$(NGINX_VERSION).tar.gz
	@echo "==> Extracting..."
	@tar -xzf $(BUILD_DIR)/nginx-$(NGINX_VERSION).tar.gz -C $(BUILD_DIR)
	@echo "==> Configuring with dynamic module..."
	@cd $(NGINX_SRC) && ./configure --with-compat --add-dynamic-module=$(ROOT) >/dev/null
	@echo "==> Building module..."
	@$(MAKE) -C $(NGINX_SRC) -s modules >/dev/null
	@echo "==> Locating built module..."
	@so_path=$$(find $(NGINX_SRC)/objs -name ngx_http_huyangix_module.so -print -quit); \
	if [ -z "$$so_path" ]; then \
		echo "Module .so not found under $(NGINX_SRC)/objs" >&2; exit 1; \
	fi; \
	 rm -f $(BUILD_DIR)/ngx_http_huyangix_module.so; \
	 cp -f "$$so_path" $(BUILD_DIR)/ngx_http_huyangix_module.so; \
	 echo "==> Module saved to $(BUILD_DIR)/ngx_http_huyangix_module.so";

build: $(SO_PATH)
	@echo "==> Module built at $(SO_PATH)"

test: build
	@echo "==> Running Test::Nginx tests..."
	@HUYANGIX_SO=$(BUILD_DIR)/ngx_http_huyangix_module.so prove -r t

clean:
	rm -rf $(BUILD_DIR)

