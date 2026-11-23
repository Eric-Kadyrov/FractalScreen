FRACTALSCREEN website: how to go live
=====================================

WHAT'S IN THIS FOLDER
  index.html          Landing page
  download.html       Download / thank-you page (starts the right file automatically)
  app.html            Web Player, free trial (3 fractals, 2 color schemes)
  app-premium.html    Web Player, Premium (everything, including music)
  manifest-*.webmanifest, sw.js   Let visitors install the Web Player as an app and use it offline
  downloads/          One-click Windows installers (Free and Premium)
  favicon-32.png, apple-touch-icon.png, logo-512.png

1. UPLOAD TO CLOUDFLARE PAGES (free)
   a. Unzip the download. Inside "fractalscreen" you'll find this "website" folder.
   b. Cloudflare dashboard > Workers & Pages > Create application > Pages >
      "Upload assets" (drag and drop). Name the project, e.g. fractalscreen.
   c. Drag the "website" folder itself into the upload box (not the "fractalscreen"
      folder around it), then Deploy. index.html must end up at the top level.
   d. Project > Settings > Variables and secrets, add:
        PADDLE_API_KEY   your Paddle API key (choose "Secret")
        PADDLE_ENV       production   (or sandbox while testing)
        PRICE_BUSINESS   your Business price ID (optional)
      Then upload the folder once more (Create deployment) so the settings take effect.
   e. Project > Custom domains > add fractalscreen.com (and www.fractalscreen.com).
   The file _worker.js is the license server. It runs automatically with drag-and-drop
   uploads and is never shown to visitors.
   Updating the site later: Project > Create deployment > drag the folder in again.

2. TURN ON PAYMENTS (Paddle)
   a. In Paddle, create a product "FRACTALSCREEN" with three prices:
        Premium yearly   $34.99 / year
        Premium monthly  $4.99 / month
        Business         $6 / month (let customers change the quantity = number of screens)
   b. Copy your client-side token (Developer tools > Authentication) and the three price IDs
      (they start with pri_).
   c. Open index.html, find FS_CONFIG near the bottom, and paste them in.
   d. In Paddle, add your website domain as an approved domain.
   After payment, the customer goes to download.html, which shows their license key
   (their Paddle subscription ID), copies it, and offers the Premium download.

3. REPLACE THE PLACEHOLDERS
   Search all .html files for "[" : [YOUR EMAIL], [YEAR], [PAYMENT PROVIDER] (write "Paddle").

4. CODE-SIGN THE WINDOWS INSTALLERS (recommended)
   Until the installers are signed, Windows SmartScreen shows "Windows protected your PC"
   (users click More info > Run anyway). A code-signing certificate removes most of this.
   Sign both FractalScreen-*-Setup.exe files before uploading.

WHAT THE WINDOWS INSTALLER DOES
   - Installs for the current user only (no admin password) into %LOCALAPPDATA%\FractalScreen
   - Sets FRACTALSCREEN as the active screensaver (5 minutes idle, unless the user already
     chose a time)
   - Adds a FRACTALSCREEN icon (with the logo) to the desktop and the Start menu.
     It opens the FRACTALSCREEN window: choose fractal, colors and speed, press Play now.
   - Adds "Play FRACTALSCREEN now" and "Uninstall FRACTALSCREEN" to the Start menu
   - Appears in Settings > Apps for uninstalling
   Installing Premium over Free simply upgrades it.

HOW THE TIME LIMITS WORK
   Free trial: 3 fractals for 14 days from the first install (Windows) or first visit (web).
     The time left is shown in the settings. After 14 days the fractals stop and a
     reminder to get Premium is shown instead. Uninstalling doesn't reset the trial.
   Premium and Business: the license key is checked with the license server. The
   screensaver runs while the subscription is paid (active, or Paddle is retrying a
   failed payment). It re-checks every 12 hours and keeps working offline for 3 days
   past the paid period. When a subscription is cancelled, it runs until the end of the
   period already paid, then stops.

WEB PLAYER
   Works in any modern browser (Mac, PC, Chromebook, tablets, smart TVs). Visitors leave it
   open; after 1 to 15 minutes without mouse, keyboard or touch activity (3 minutes by
   default) it goes full screen with fractals and keeps the screen awake while playing.
   Browsers only let a page react while it is open and visible, so it can't start when the
   tab is closed or minimised. That's why the desktop app is recommended on Windows.

CONTACT FORM
   contact.js shows the form; contact.php emails each message to contact@fractalscreen.com,
   with the visitor's address as Reply-To, so you can simply press Reply.
   Needs: PHP hosting (Hostinger has it) and a mailbox contact@fractalscreen.com created in
   Hostinger > Emails, because messages are sent from that address.
   Spam protection: a hidden robot trap and a limit of 5 messages per visitor per hour.

GOOD TO KNOW
   - The installer files themselves can be shared, but Premium won't run without a paid key.
     One key can currently be used on any number of computers.
   - A determined user could reset the free trial by editing the Windows registry or
     clearing browser data. That's normal for simple trials; stronger protection needs
     user accounts.
   - Windows Premium doesn't have music yet (the Premium web version does).
   - Mac, Android and iPhone versions are marked "coming soon" on the site.
