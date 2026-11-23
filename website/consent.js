/* FRACTALSCREEN cookie consent.
   Google Analytics starts with storage denied (see the consent lines in each page's <head>)
   and is only allowed after the visitor clicks Accept. Any element with
   data-cookie-settings reopens this banner. */
(function () {
  var KEY = "fs-consent";
  function get() { try { return localStorage.getItem(KEY); } catch (e) { return null; } }
  function set(v) { try { localStorage.setItem(KEY, v); } catch (e) {} }
  function apply(v) {
    if (typeof window.gtag !== "function") return;
    window.gtag("consent", "update", { analytics_storage: v === "granted" ? "granted" : "denied" });
  }

  var css = document.createElement("style");
  css.textContent =
    "#fs-consent{position:fixed;left:16px;right:16px;bottom:16px;z-index:900;max-width:720px;margin:0 auto;box-sizing:border-box;padding:18px 20px;border-radius:16px;background:#181B45;color:#F1EEFF;border:1px solid rgba(241,238,255,.2);box-shadow:0 18px 40px -12px rgba(0,0,0,.6);display:flex;gap:16px;align-items:center;flex-wrap:wrap;font-family:'Instrument Sans',system-ui,-apple-system,'Segoe UI',sans-serif;font-size:15px;line-height:1.5}" +
    "#fs-consent[hidden]{display:none}" +
    "#fs-consent p{margin:0;flex:1 1 300px;color:#CFCDEA}" +
    "#fs-consent a{color:#FFC27A}" +
    "#fs-consent .fs-c-btns{display:flex;gap:10px;flex-wrap:wrap}" +
    "#fs-consent button{font:inherit;font-weight:600;min-height:44px;padding:0 20px;border-radius:999px;cursor:pointer}" +
    "#fs-consent .fs-c-yes{border:0;background:#FFC27A;color:#101233}" +
    "#fs-consent .fs-c-no{background:transparent;color:#F1EEFF;border:1.5px solid rgba(241,238,255,.45)}" +
    "#fs-consent button:focus-visible{outline:3px solid #FFC27A;outline-offset:3px}";
  document.head.appendChild(css);

  var box = document.createElement("div");
  box.id = "fs-consent";
  box.setAttribute("role", "region");
  box.setAttribute("aria-label", "Cookie choice");
  box.hidden = true;
  box.innerHTML =
    '<p>We’d like to use Google Analytics cookies to see how visitors use FRACTALSCREEN, so we can improve it. ' +
    'Nothing is shared for advertising. <a href="privacy.html#cookies">Privacy policy</a></p>' +
    '<div class="fs-c-btns"><button type="button" class="fs-c-no">Decline</button><button type="button" class="fs-c-yes">Accept</button></div>';
  document.body.appendChild(box);

  function choose(v) { set(v); apply(v); box.hidden = true; }
  box.querySelector(".fs-c-yes").addEventListener("click", function () { choose("granted"); });
  box.querySelector(".fs-c-no").addEventListener("click", function () { choose("denied"); });

  document.addEventListener("click", function (e) {
    var t = e.target.closest && e.target.closest("[data-cookie-settings]");
    if (!t) return;
    e.preventDefault();
    box.hidden = false;
    box.querySelector(".fs-c-yes").focus();
  });

  if (!get()) box.hidden = false;
})();
