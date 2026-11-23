/* ===== FRACTALSCREEN site settings ==========================================
   Paste your Stripe links here after creating them in the Stripe Dashboard.
   Payment Links: Stripe > Payment Links > (link) > Copy link (starts https://buy.stripe.com/)
   Customer portal: Stripe > Settings > Billing > Customer portal > Login link
   (starts https://billing.stripe.com/p/login/)
   ========================================================================== */
var FS_CONFIG = {
  checkout: {
    "premium-yearly":  "",   // $34.99 / year
    "premium-monthly": "",   // $4.99 / month
    "business":        ""    // $6 / screen / month
  },
  portal: ""                 // where customers manage or cancel their subscription
};
/* ========================================================================== */
(function () {
  var isWindows = /Windows NT/i.test(navigator.userAgent);
  function isStripe(url, kind) {
    return kind === "portal" ? /^https:\/\/billing\.stripe\.com\//.test(url || "")
                             : /^https:\/\/(buy|checkout)\.stripe\.com\//.test(url || "");
  }
  function contact(topic) { if (window.FS_contact) window.FS_contact(topic); else location.href = "mailto:contact@fractalscreen.com"; }

  // Free downloads go through the download page, which starts the file and shows the install steps
  var freeLinks = document.querySelectorAll('a[href$="FractalScreen-Free-Setup.exe"]');
  for (var i = 0; i < freeLinks.length; i++) { freeLinks[i].href = "download.html?plan=free"; freeLinks[i].removeAttribute("download"); }

  // Mac, phones, tablets: the Web Player is the main button, the Windows download the second one
  var cta = document.getElementById("fs-hero-cta"), label = document.getElementById("fs-hero-cta-label"), alt = document.getElementById("fs-hero-alt");
  if (cta && !isWindows) {
    cta.href = "app.html"; cta.removeAttribute("download");
    if (label) label.textContent = "Open the Web Player";
    var svg = cta.querySelector("svg"); if (svg) svg.style.display = "none";
    if (alt) { alt.href = "download.html?plan=free"; alt.textContent = "Download for Windows"; }
  }

  // Checkout buttons go to Stripe. Stripe sends the customer back to download.html with their purchase.
  var buttons = document.querySelectorAll("[data-checkout]");
  for (var j = 0; j < buttons.length; j++) {
    var link = FS_CONFIG.checkout[buttons[j].getAttribute("data-checkout")];
    if (isStripe(link)) { buttons[j].href = link; continue; }
    buttons[j].addEventListener("click", function (e) {   // not set up yet: let the visitor ask instead of a dead button
      e.preventDefault();
      contact(/business/.test(this.getAttribute("data-checkout")) ? "Business plan" : "Premium");
    });
  }

  // "Manage subscription" opens the Stripe customer portal
  var portals = document.querySelectorAll("[data-portal]");
  for (var k = 0; k < portals.length; k++) {
    if (isStripe(FS_CONFIG.portal, "portal")) { portals[k].href = FS_CONFIG.portal; portals[k].target = "_blank"; portals[k].rel = "noopener"; continue; }
    portals[k].addEventListener("click", function (e) { e.preventDefault(); contact("Manage subscription"); });
  }
})();
