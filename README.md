# ХУЯНГИКС — Huyangix NGINX Module

![Logo](./logo.png)

> _“Because your web server deserves a little chaos.”_  
> — The Dev

**Huyangix** is a lightweight NGINX module that brings a touch of unpredictability and Russian-style fun to your HTTP responses. Whether you're building a dev environment full of Easter eggs or just want your 404 pages to be hilariously random, this module’s got your back.

---

## 🧠 What is Huyangix?

Huyangix is a plug-in module for NGINX that injects randomized behavior into your web server responses, such as:

- Randomized 404 error pages with witty Russian-flavored messages.
- Easter egg responses on certain holidays (e.g., April Fool’s, New Year).
- Developer mode to prank your co-workers (because why not?).

This module is **not meant for production** — unless your production team is into chaos and humor.

---

## 🔧 Installation

> ⚠️ This module requires NGINX to be compiled from source with dynamic module support.

```bash
# Clone the repo
git clone https://github.com/yourusername/huyangix-nginx-module.git
cd nginx-x.y.z  # Your NGINX source dir

# Configure with module
./configure --add-dynamic-module=../huyangix-nginx-module
make modules
cp objs/ngx_http_huyangix_module.so /etc/nginx/modules/
```

---

## 🧪 Usage

Add this to your NGINX config:

```nginx
load_module modules/ngx_http_huyangix_module.so;

http {
    server {
        listen 80;
        server_name localhost;

        location / {
            huyanginx on;
        }
    }
}
```

### Optional directives

```nginx
huyanginx on;                               # Enable the module
huyangix_message_file /path/to/messages.txt;  # Optional: custom messages
```

---

## 🎭 Example Output

Random 404s might look like:

- `404 Хуй тебе, а не страница!`
- `Oops! Где-то ты свернул не туда.`
- `Сервер устал. Иди попей чай.`

---

## 🤓 Internals

Under the hood, Huyangix works by hooking into the response phase and replacing normal 404 outputs with a randomized pick from a preset (or user-defined) list of messages.

If `huyangix_message_file` is not provided, a default set of Russian-language errors will be used.

---

## 🚧 Status

- [x] Basic module skeleton
- [x] Randomized 404 handler
- [ ] Custom status code support
- [ ] Web UI for message editing (maybe?)
- [ ] Easter egg triggers

---

## 🇷🇺 Name Meaning?

Let's just say the name is a "very expressive" blend of Russian slang and nginx — don't use it in polite company 😏

---

## 📜 License

BSD 2-Clause — same as NGINX.  
Use at your own risk. Laughs guaranteed, uptime not.

---

## 🐻 Logo

Designed with love, vodka, and a bear in a шапка-ушанка. See `logo.png`.

---

## 👏 Acknowledgements

Thanks to:
- The NGINX devs
- Russian humor
- The glorious internet

---

**Stars are appreciated. Forks are welcome. Use responsibly.**
