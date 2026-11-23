<?php
// FRACTALSCREEN license server settings.
// 1. Copy this file to config.php (same folder) on your server.
// 2. Fill in your Stripe key below. config.php is never committed to GitHub.

return [
    // Stripe > Developers > API keys. Best: a "restricted key" (starts with rk_live_)
    // with READ access to: Checkout Sessions, Subscriptions. Nothing else is needed.
    // For testing use your test-mode key (rk_test_ / sk_test_).
    'stripe_secret_key' => 'rk_live_REPLACE_ME',

    // Price IDs of the Business plan (Stripe > Product catalog > Business > price ID, starts with price_),
    // so Business customers are recognised. Optional.
    'business_price_ids' => [],
];
