FRACTALSCREEN website: launch guide (Hostinger + Stripe)
========================================================

WHAT'S IN THIS FOLDER
  index.html              Landing page
  app.html                Web Player, free trial (3 fractals, 2 color schemes, 4 weeks)
  app-premium.html        Web Player, Premium (everything, including music)
  download.html           Download / thank-you page; shows the license key after checkout
  terms.html, privacy.html, refund.html, 404.html
  downloads/              One-click Windows installers (Free and Premium)
  api/                    License server (PHP): checks Stripe subscriptions
  contact.php, contact.js Contact form, emails contact@fractalscreen.com
  site.js                 Stripe checkout links and "Manage subscription" link  <-- you edit this
  music.js                Cosmic focus music for the Web Player (generated live, follows device volume)
  consent.js              Cookie banner (Google Analytics only after "Accept")
  sw.js, manifest-*.webmanifest   Install the Web Player as an app / offline use
  .htaccess, robots.txt, sitemap.xml

1. UPLOAD
   Upload everything in this folder into public_html on Hostinger, keeping the
   folders. Hostinger's File Manager hides files starting with a dot: turn on
   "Show hidden files" and check that .htaccess, api/.htaccess and
   downloads/.htaccess are there.
   In Hostinger, make sure SSL is active and "Force HTTPS" is on.

2. EMAIL
   Hostinger > Emails: create the mailbox contact@fractalscreen.com.
   The contact form sends from it, and customers write to it.

3. STRIPE (do it in Test mode first, then repeat in Live mode)
   a. Product catalog > add products with recurring prices:
        FRACTALSCREEN Premium   $4.99 / month   and   $34.99 / year
        FRACTALSCREEN Business  $6 / month (per screen)
   b. Payment Links > create one link per price:
        Premium yearly, Premium monthly, Business (turn on "Let customers adjust
        quantity" for Business, so they can choose the number of screens).
      For each link, under "After payment" choose "Don't show confirmation page,
      redirect customers to your website" and enter:
        Premium links:  https://fractalscreen.com/download.html?plan=premium&session_id={CHECKOUT_SESSION_ID}
        Business link:  https://fractalscreen.com/download.html?plan=business&session_id={CHECKOUT_SESSION_ID}
      (type {CHECKOUT_SESSION_ID} exactly like that; Stripe fills it in)
   c. Settings > Billing > Customer portal: activate it, allow customers to cancel,
      and copy the "Login link" (https://billing.stripe.com/p/login/...).
   d. Open site.js and paste the three payment links and the portal link at the top.
   e. Developers > API keys > Create restricted key, with Read access to
      "Checkout Sessions" and "Subscriptions" only. Copy it.
   f. In api/, copy config.example.php to config.php and paste the key into it.
      (config.php is private: the server never shows it, and it's not in GitHub.)
      For Business customers to be recognised, also add the Business price ID.
   g. Settings > Business > Public details: add the website, support email
      contact@fractalscreen.com, and link Terms, Privacy and Refund pages.

4. TEST (Stripe Test mode, card 4242 4242 4242 4242, any future date, any CVC)
   - Buy Premium monthly on fractalscreen.com -> you land on download.html and see a
     key starting with sub_.
   - Open https://fractalscreen.com/api/verify.php?key=<that key>
     -> {"active":true,...}
   - Install the Premium Windows app with the key, and open app-premium.html: both
     should activate.
   - Cancel in the customer portal: Premium keeps working until the period ends.
   Then switch site.js links and config.php key to Live mode.

5. CHECK
   - Footer links: Terms, Privacy, Refunds, Manage subscription, Cookie settings.
   - Contact form: send yourself a message.
   - Google Analytics > Reports > Realtime: accept cookies on the site, you should
     appear within a minute.

HOW THE TIME LIMITS WORK
   Free trial: 3 fractals for 4 weeks (28 days) from the first install (Windows) or
     first visit (Web Player). The time left is shown in the settings. Afterwards the
     fractals stop and a reminder to get Premium is shown. Uninstalling doesn't reset it.
   Premium and Business: the license key is the Stripe subscription ID (sub_...).
     The app checks it with api/verify.php every 12 hours, and keeps working offline
     for 3 days past the paid period. Active means: active, trialing or past_due
     (Stripe is retrying a payment). After cancelling, it runs until the end of the
     period already paid, then stops.

WHAT THE WINDOWS INSTALLER DOES
   - Installs for the current user only (no admin password) into %LOCALAPPDATA%\FractalScreen
   - Sets FRACTALSCREEN as the active screensaver (5 minutes idle, unless already set)
   - Adds a FRACTALSCREEN icon (with the logo) to the desktop and the Start menu.
     It opens the FRACTALSCREEN window: choose fractal, colors, speed, press Play now.
   - Adds "Play FRACTALSCREEN now" and "Uninstall FRACTALSCREEN" to the Start menu
   - Premium: asks for the license key (filled in automatically from the download page)
   - Plays cosmic focus music in full screen (not in the small preview). Volume follows
     Windows; it can be switched off in FRACTALSCREEN settings.
   Installing Premium over Free simply upgrades it.

GOOD TO KNOW
   - Until the installers are code-signed, Windows SmartScreen shows "Windows protected
     your PC" (More info > Run anyway), and Hostinger's malware scanner may quarantine
     them. A code-signing certificate fixes both.
   - One license key works on any number of the customer's devices.
   - A determined user could reset the free trial by editing the registry or clearing
     browser data. Normal for simple trials.
