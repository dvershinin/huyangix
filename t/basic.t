use Test::Nginx::Socket 'no_plan';
use utf8;
use Cwd qw(cwd abs_path);

BEGIN {
	my $so = $ENV{HUYANGIX_SO} || 'build/ngx_http_huyangix_module.so';
	if ($so !~ m{^/}) {
		$so = cwd . "/$so";
	}
	$ENV{TEST_NGINX_HUYANGIX_SO} = $so;
}

run_tests();

__DATA__
=== TEST 1: enabled with single custom message (deterministic)
--- main_config
load_module $TEST_NGINX_HUYANGIX_SO;
--- user_files
>>> /tmp/huyangix_messages.txt
Privet
--- config
location /t1 {
    huyanginx on;
    huyangix_message_file /tmp/huyangix_messages.txt;
    return 404;
}
--- request
GET /t1
--- error_code: 404
--- response_body_like: ^404 Privet$
=== TEST 2: pass-through non-404
--- main_config
load_module $TEST_NGINX_HUYANGIX_SO;
--- config
location /ok {
    huyanginx on;
    return 200 "fine";
}
--- request
GET /ok
--- error_code: 200
--- response_body_like: ^fine$
