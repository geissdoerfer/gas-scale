# Android app plan — gas bottle monitor (account-less, single device)

Companion app for a Verolic gas bottle monitor. No user accounts: the device pairing
secret *is* the credential. One app install is paired to exactly one device at a time.

## 1. User-facing flow

```
Install → [Unpaired] → Pair (scan QR / manual entry) → Pick cylinder type → [Paired] → Main gauge
                                                                                   │
                                                                     Unpair ───────┘
                                                                     (back to Unpaired)
```

1. **Pairing screen** (initial screen, and again after unpairing) — camera view for scanning
   the device's QR code, plus a "enter manually" fallback (two text fields: device ID,
   secret). Either path ends with the same pair request.
2. **Cylinder picker** — shown immediately after a successful pairing, and reachable later
   from the main screen to change the type. Four fixed choices: **3 kg / 6 kg / 12 kg / 45 kg**.
   Selecting one calls the server immediately (no separate "confirm" step needed) and the
   choice is echoed back from the server so the app is never the source of truth for it.
3. **Main screen** — a fill gauge (0–100%, from the server) and a battery indicator. Pull-to-refresh
   plus periodic polling while visible. Overflow/settings menu has "Change cylinder type"
   (→ picker, pre-filled with current selection) and "Unpair".
4. **Unpair** — confirmation dialog, then: clear the locally stored device ID/token, return
   to the Pairing screen. Purely a local state reset (see §3.3 on why the server doesn't need
   to know).

State is a strict two-state machine at the app level — `Unpaired` / `Paired` — with
"cylinder type not yet chosen" as a transient sub-state of `Paired` entered only once, right
after the first successful pair.

## 2. Backend changes required (blocking — none of this exists yet)

The current `web-api` is entirely user/JWT based (`docs/API.md`): admin creates users,
assigns devices, users log in with username+password. This flow has no user at all, so it
needs its own auth path rather than reusing `/auth/login`.

### 2.1 Schema (`devices` table)

| Column | Type | Notes |
|---|---|---|
| `pairing_secret` | `VARCHAR(64)`, unique, not null | Random, generated at provisioning time, printed as the device's QR code. Static — not consumed/invalidated by pairing, so re-pairing (new phone, factory reset of the app) always works with the same label. |
| `cylinder_type` | `VARCHAR(10)`, nullable | One of `3kg` / `6kg` / `12kg` / `45kg`. Null until the app completes step 2 of onboarding. |

Migration follows the style of `database/migrations/001_add_calibration.sql` (idempotent
`ADD COLUMN IF NOT EXISTS`). `sensor_readings`, `users`, `user_device_access` are untouched —
the existing admin/user system keeps working for ops purposes (support access, fleet
provisioning) independent of this flow.

A small **cylinder spec table**, keyed by `cylinder_type`, drives the percentage math:

| type | tare (empty) kg | full (gross) kg | — |
|---|---|---|---|
| `3kg` | *TBD* | *TBD* | placeholder, needs real spec |
| `6kg` | *TBD* | *TBD* | placeholder, needs real spec |
| `12kg` | *TBD* | *TBD* | placeholder, needs real spec |
| `45kg` | *TBD* | *TBD* | placeholder, needs real spec |

**Open question — not guessed here:** actual tare/full weights for your specific cylinder
brands/models. This table can start as a Python constant in the API (not a DB table — only
4 fixed rows, no admin UI needed for it) and move to a real table later if sizes become
configurable.

### 2.2 Device-scoped auth (new, parallel to the existing user-JWT auth)

- **`POST /devices/pair`** — public (no bearer token). Body `{device_id, secret}`. Validates
  against `pairing_secret`; on match, issues a **device token**: a JWT with
  `{"sub": device_id, "scope": "device"}`, long-lived (e.g. 1 year — there's no refresh flow
  for an account-less token, so it should outlive the phone's expected re-pair interval, or
  the app should just re-pair silently on 401 using the still-known secret if you'd rather
  keep it short-lived — pick one, see open question below). 404 on unknown `device_id`,
  401 on secret mismatch.
- A new `get_current_device` dependency in `dependencies.py`, parallel to `get_current_user`:
  decodes the bearer token, requires `scope == "device"`, loads the `Device` row from `sub`.
  Every endpoint below requires this and implicitly scopes to that one device — there is no
  cross-device access check to write (unlike `check_device_access`) because the token *is*
  the device identity.
- **Open question:** should `pairing_secret` ever be treated as sensitive enough to rotate
  (e.g. a "reset pairing" admin action that invalidates all outstanding device tokens for
  that device)? Not needed for MVP; flagging since it affects whether device tokens need a
  revocation/short-TTL story now or can stay simple.

### 2.3 Cylinder type + status endpoints (device-token protected)

- **`PATCH /devices/me/cylinder`** — body `{cylinder_type}` (validated against the 4 allowed
  values), updates the row for the calling device, returns the updated status (see below).
  Using `/devices/me/...` rather than `/devices/{device_id}/...` avoids the app needing to
  know/send its own ID once paired — the token already identifies it.
- **`GET /devices/me/status`** — the one endpoint the main screen polls. Returns:
  ```json
  {
    "cylinder_type": "12kg",
    "percentage_remaining": 63,       // null if cylinder_type not yet set, or no reading yet
    "battery_percentage": 82,          // see open question below
    "last_updated": "2026-09-13T10:02:00Z",
    "status": "ok"                     // ok | low_battery | not_configured | no_data
  }
  ```
  `percentage_remaining` computed server-side as today's calibration is:
  `weight = (raw_value + device.offset) * device.gain` (existing), then
  `clamp((weight - tare_kg) / (full_kg - tare_kg) * 100, 0, 100)` using the cylinder spec
  table from §2.1. The app never sees `raw_value`/`weight`/tare/full — only the final
  percentage, per your requirement that this calculation stay server-side.
- **Battery: raw `battery_voltage` is stored today, not a percentage.** Showing a charge
  *percentage* (as you described) needs a voltage→percentage curve, which needs the battery
  chemistry/discharge curve to be accurate. **Open question:** do you have that curve, or
  should MVP show raw voltage instead of a computed percentage? Plan assumes a percentage is
  wanted and treats the curve as a placeholder constant (e.g. linear between two configured
  voltage bounds) to be tuned once real discharge data exists.

No `unpair` endpoint is strictly needed — see §3.3. If you'd like the server to be able to
force-unpair a device (e.g. support tooling), that's a separate admin-authenticated action on
the existing `PUT /devices/{device_id}` surface, not something the app calls.

### 2.4 What stays out of scope

`POST /auth/register`, `/auth/login` self-service, and the old blueprint's multi-device
dashboard are **not** built — they belonged to the earlier accounts-based design this plan
replaces per your direction. The existing admin-only user system stays as-is for ops.

## 3. Android app architecture

### 3.1 Stack

Same baseline as the previous attempt (Compose template, package `com.example.verolic`,
no networking/DI yet):

- **UI**: Jetpack Compose + Material3, Navigation-Compose (3 destinations: Pairing, Cylinder
  picker, Main — no bottom nav, no device list).
- **Networking**: Retrofit + OkHttp + kotlinx.serialization.
- **Async/state**: Coroutines + `StateFlow`, `collectAsStateWithLifecycle`.
- **DI**: skip Hilt for something this small; a hand-rolled `AppContainer` (as the previous
  attempt had) is enough for 1–2 repositories.
- **Token storage**: `androidx.security.crypto` `EncryptedSharedPreferences` — the device
  token is a bearer credential even without a "user", so it gets the same treatment a
  user JWT would.
- **QR scanning**: CameraX (`PreviewView` + `ImageAnalysis`) + ML Kit Barcode Scanning.
- **Testing**: JUnit + MockK for the repository/ViewModels against a fake `DeviceApi`;
  Compose UI test for the pair → picker → main happy path.

### 3.2 QR payload format

Reuse the scheme from the earlier attempt (`android/blueprint.md`, now superseded but the
format is still sound): `verolic://pair?device_id=<id>&secret=<pairing_secret>`.
`PairingPayload.parse(raw)` / `.encode(...)` port over unchanged. Manual entry produces the
same `PairingPayload` value from two text fields, so both paths feed one code path.

### 3.3 Local state & the "unpair" decision

The app persists exactly one thing in `EncryptedSharedPreferences`: the paired device's
`device_id` + device token (or nothing, if unpaired). `Unpair` clears it and nothing else —
no server call required, because:
- the `pairing_secret` is static and reusable (§2.1), so pairing again later just works;
- there's no per-user assignment record to release, since there's no user;
- if you later want "unpair" to also revoke the specific token being discarded (defense in
  depth against a stolen/cached token), that needs the revocation mechanism flagged as an
  open question in §2.2 — add it then, skip it for MVP.

### 3.4 Layers

Single `:app` module, package-by-feature (`pairing`, `device`, `common/network`, `common/ui`).

- **`PairingRepository`**: wraps `POST /devices/pair`; on success writes the device token to
  the encrypted store and flips a `StateFlow<PairState>` (`Unpaired` / `Paired(deviceId)`)
  that the root `NavHost` observes to pick the start destination (mirrors how the old
  `AuthState` drove nav, just without a user).
- **`DeviceRepository`**: wraps `PATCH /devices/me/cylinder` and `GET /devices/me/status`,
  both authenticated via an OkHttp interceptor that attaches the stored device token. A 401
  here (token invalid/expired) is treated the same as "unpaired" — clear local state and
  bounce to the Pairing screen, since there's no refresh flow to fall back to.
- ViewModel per screen, each exposing one `StateFlow<UiState>` (`Loading` / `Success` /
  `Error(message)`), same pattern as the earlier attempt.

### 3.5 Screens / nav graph

1. **Splash** (no visible UI) — reads persisted pair state once, routes to Pairing or Main.
2. **Pairing** — CameraX preview + ML Kit scanner, `CAMERA` permission via
   `rememberLauncherForActivityResult`; "Enter manually" toggles to two text fields
   (device ID, secret) as a fallback for damaged/unreadable labels. Either path calls
   `PairingRepository.pair(payload)`. On success → Cylinder picker (first-time) since
   `cylinder_type` is always null right after a fresh pair.
3. **Cylinder picker** — 4 radio-style cards (3/6/12/45 kg). Calls
   `DeviceRepository.setCylinderType(...)` on selection; on success → Main. Also reachable
   from Main's menu to change the type later (pre-selects current value from `GET .../status`).
4. **Main** — big circular fill gauge (`percentage_remaining`) + battery indicator
   (`battery_percentage` or raw voltage, pending the open question in §2.3), last-updated
   timestamp, status badge for `low_battery` / `no_data`. Polls `GET /devices/me/status`
   every ~30s while visible (`repeatOnLifecycle(STARTED)`) plus pull-to-refresh. Overflow
   menu: "Change cylinder type", "Unpair" (confirmation dialog first — this is destructive
   from the user's point of view even though it's just local state).

### 3.6 Security notes

- HTTPS only in production; dev `baseUrl` via `BuildConfig` per build type.
- The pairing secret is entered/scanned once, sent over the pair request, and never logged
  or displayed again client-side.
- Device token stored encrypted, attached only via the OkHttp interceptor (never logged).

### 3.7 Implementation phases

1. **Setup** — dependencies above, `BuildConfig` base URL, network security config.
2. **Backend** — §2 changes (migration, `/devices/pair`, `get_current_device`, cylinder
   endpoints, status endpoint with server-side percentage math). The app can't be
   meaningfully tested end-to-end before this lands; stub it with a fake `DeviceApi` in the
   meantime if you want the UI work to proceed in parallel.
3. **Pairing** — encrypted token store, `PairingRepository`, camera permission flow, QR
   scanner + manual-entry screen, pair-state-aware `NavHost`.
4. **Cylinder + main screen** — `DeviceRepository`, picker screen, main gauge screen with
   polling/pull-to-refresh, unpair flow.
5. **Polish** — error/empty/loading/`not_configured`/`no_data` states, dark theme, launcher
   icon/branding, accessibility (content descriptions on the gauge, dynamic type).

## 4. Open questions to resolve before/while building §2

1. Real tare/full weight per cylinder type (3/6/12/45 kg) — needed for the percentage
   formula; currently placeholders.
2. Battery: show a computed percentage (needs a voltage→charge curve) or raw voltage for MVP?
3. Device token lifetime/refresh story — long-lived static token vs. some renewal mechanism.
4. Whether `pairing_secret` ever needs rotation/revocation (support scenario: lost/stolen
   phone, want to invalidate its access without physically relabeling the device).
