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
- `download.html`: download and thank-you page; shows the license key after checkout
- `downloads/`: the Windows installers `FractalScreen-Free-Setup.exe` and `FractalScreen-Premium-Setup.exe`
- `contact.php`, `contact.js`: contact form that emails contact@fractalscreen.com
- `_worker.js`: license server for Cloudflare Pages (checks subscriptions with the payment provider)
- `sw.js`, `manifest-*.webmanifest`: let visitors install the Web Player as an app
- `README.txt`: deployment and setup notes

### `windows/`

- `fractalscreen.c`: the screensaver (OpenGL fractals, settings window, free trial timer, Premium license check)
- `installer.nsi`: NSIS installer: sets the screensaver, adds desktop and Start menu icons
- `res.rc`, `fractalscreen.ico`: icon and version info
- `BUILD.txt`: how to build the `.scr` files and installers (MinGW-w64 + NSIS)

## Plans

- **Free trial**: 3 fractals for 14 days
- **Premium**: $4.99/month or $34.99/year, all fractals and colors, music, runs while subscribed
- **Business**: $6 per screen per month, for screens in public spaces

## Status

- Payments: checkout buttons are wired for Paddle and still need to be switched to Stripe.
- Analytics: Google tag `G-26B6J0MX81` on every page.
- Mac, Android and iPhone apps: planned.
