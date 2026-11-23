<?php
// GET /api/license.php?session_id=cs_...  ->  {"key":"sub_...","active":true,"until_unix":...,"plan":"premium"}
// Called by download.html right after Stripe checkout, to show the customer their license key.
// Returns 202 {"pending":true} while Stripe is still finishing the subscription.
require __DIR__ . '/stripe.php';

if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'GET') fs_json(['error' => 'method_not_allowed'], 405, false);

$sid = trim((string)($_GET['session_id'] ?? ''));
if (!preg_match('/^cs_(test|live)_[A-Za-z0-9]{10,200}$/', $sid)) fs_json(['error' => 'invalid_session'], 400, false);

fs_rate_limit('license', 60);

[$status, $session] = stripe_get('checkout/sessions/' . rawurlencode($sid));
if ($status === 404) fs_json(['error' => 'not_found'], 404, false);
if ($status !== 200 || !is_array($session)) fs_json(['error' => 'server'], 502, false);

$paid = in_array($session['payment_status'] ?? '', ['paid', 'no_payment_required'], true);
$subRef = $session['subscription'] ?? null;
if (($session['status'] ?? '') !== 'complete' || !$paid || !$subRef) fs_json(['pending' => true], 202, false);

$subId = is_array($subRef) ? (string)($subRef['id'] ?? '') : (string)$subRef;
[$status, $sub] = stripe_get('subscriptions/' . rawurlencode($subId));
if ($status !== 200 || !is_array($sub)) fs_json(['pending' => true], 202, false);

fs_json(array_merge(['key' => $sub['id']], fs_summary($sub)), 200, false);
