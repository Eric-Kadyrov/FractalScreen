# FRACTALSCREEN

Colorful, endlessly moving fractal screensavers with calm, generated focus music.
Live at **https://fractalscreen.com**.

## What's in this repository

| Folder | What it is |
|---|---|
| `website/` | The complete website, ready to upload to the hosting root (`public_html` on Hostinger). |
| `windows/` | Source for the native Windows screensaver and its one-click installers (Free and Premium). |
| `prototype/` | The original single-page fractal screensaver with music. |

### `website/`

- `index.html`: landing page (fractal gallery, pricing, FAQ, contact form)
- `app.html`, `app-premium.html`: the Web Player (free 2-week trial, and Premium)
- `download.html`: download and thank-you page; shows the license key after Stripe checkout
- `terms.html`, `privacy.html`, `refund.html`, `404.html`: legal and error pages
- `downloads/`: the Windows installers `FractalScreen-Free-Setup.exe` and `FractalScreen-Premium-Setup.exe`
- `contact.php`, `contact.js`: contact form that emails contact@fractalscreen.com
- `api/`: license server in PHP (`verify.php`, `license.php`): checks Stripe subscriptions. Copy `config.example.php` to `config.php` on the server and add the Stripe key; `config.php` is never committed.
- `site.js`: Stripe Payment Links and customer portal link
- `consent.js`: cookie banner; Google Analytics runs only after visitors accept
- `sw.js`, `manifest-*.webmanifest`: let visitors install the Web Player as an app
- `README.txt`: step-by-step launch guide (Hostinger, Stripe, testing)

### `windows/`

- `fractalscreen.c`: the screensaver (OpenGL fractals, settings window, free trial timer, Premium license check)
- `installer.nsi`: NSIS installer: sets the screensaver, adds desktop and Start menu icons
- `res.rc`, `fractalscreen.ico`: icon and version info
- `BUILD.txt`: how to build the `.scr` files and installers (MinGW-w64 + NSIS)

## Plans

- **Free trial**: 3 fractals for 4 weeks, Windows app and Web Player
- **Premium**: $4.99/month or $34.99/year, all fractals and colors, music, runs while subscribed
- **Business**: $6 per screen per month, for screens in public spaces

## Status

- Payments: Stripe Payment Links and customer portal; license key = Stripe subscription ID.
- Hosting: Hostinger (PHP).
- Analytics: Google tag `G-26B6J0MX81` on every page, with cookie consent.
- Mac, Android and iPhone apps: planned.
