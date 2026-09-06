# Android app

Companion app for a gas bottle monitor.

## Key functionality
 - There exists a server (fastapi/restful)
 - User creates an account
 - User logs in and out of their account
 - User adds a device to their account by scanning a QR code
 - User provides the tara weight of their gas bottle (gets stored on the server)
 - User picks the gas bottle type from a preset list (defines the nominal full-gas weight used together with tara to compute %)
 - App shows the percentage of gas remaining in the bottle. This is computed on the server and only shown on the device.

---

## Backend API changes required

The current `web-api` (see `../API_SUMMARY.md`, `../docs/API.md`) is admin-provisioned: an admin creates users and assigns devices to them. The blueprint requires self-service signup and self-service device pairing, plus two pieces of data the current `Device` model doesn't have. These are backend changes, not Android changes, but the app can't be built without them, so they're captured here.

1. **`POST /auth/register`** — `{username, email, password}` → creates a `role="user"` account, returns the same token pair shape as `/auth/login`. Same validation rules as `POST /users` (username 3-50 chars, password ≥ 8 chars).

2. **Device pairing secret + self-claim endpoint.** `device_id` alone isn't secret (it's visible in MQTT topics, docs, etc.), so self-service claiming can't just take a `device_id` like the admin `/assign` endpoint does. Add a random `pairing_secret` column to `devices`, generated at provisioning time and printed as a QR code on the device label. New endpoint:
   - **`POST /devices/claim`** — authenticated as the logged-in user, body `{device_id, pairing_secret}`. Validates the secret, creates the user↔device assignment (same effect as admin `/assign`), 404/401 on mismatch. QR payload proposal: `verolic://pair?device_id=<id>&secret=<pairing_secret>`.

3. **Tara weight + bottle type on the device.** Add `tare_weight_kg` (float, nullable) and `bottle_type` (string/enum FK, nullable) to `devices`. A small seed table of bottle types (`name`, `nominal_fill_kg`) backs the picker — e.g. common propane/camping-gas cylinder sizes; exact list TBD with the user.
   - **`PATCH /devices/{device_id}/config`** — user-scoped (not admin-only like `PUT /devices/{device_id}`), restricted to `{tare_weight_kg, bottle_type}`, only for devices assigned to the caller.
   - **`GET /bottle-types`** — list of `{id, name, nominal_fill_kg}` for the picker.

4. **Computed percentage.** Extend `latest`, `readings`, and `dashboard` responses with `percentage_remaining`: `clamp(((load - tare_weight_kg) / nominal_fill_kg) * 100, 0, 100)`. Null with a `status: "not_configured"` when `tare_weight_kg` or `bottle_type` isn't set yet — the app must handle this state (prompt the user to finish setup) rather than assume every paired device has a percentage.

Existing admin endpoints (`POST /users`, `POST /devices`, `POST /devices/{id}/assign`, etc.) stay as-is for support/ops use; the above are additive.

---

## Android app architecture

### Stack

The project is a stock Android Studio Compose template (package `com.example.verolic`, minSdk 24, targetSdk/compileSdk 37, no networking/DI yet). Proposed additions, kept minimal for an app this size:

- **UI**: Jetpack Compose + Material3 (already scaffolded), Navigation-Compose for the nav graph.
- **DI**: Hilt.
- **Networking**: Retrofit + OkHttp + kotlinx.serialization.
- **Async/state**: Kotlin Coroutines + `StateFlow`, `collectAsStateWithLifecycle` in Compose.
- **Auth token storage**: `androidx.security.crypto` (`EncryptedSharedPreferences`) — tokens are sensitive, plain `DataStore` isn't enough.
- **QR scanning**: CameraX (`PreviewView` + `ImageAnalysis`) + ML Kit Barcode Scanning (on-device, no extra permissions beyond camera).
- **Testing**: JUnit + MockK for ViewModels/repositories (fake API service, no real network in tests), Compose UI tests for login/pairing happy paths.

No Room/offline cache for MVP — the blueprint doesn't call for offline use, and a `StateFlow` cache in the repository (last successful response, cleared on logout) is enough. Revisit only if a real offline requirement shows up.

### Layers

Single `:app` module, package-by-feature (`auth`, `devices`, `pairing`, `common/network`, `common/ui`, `di`). No separate domain/use-case layer — ViewModels talk to repositories directly; introduce use cases later only if logic starts getting duplicated across ViewModels.

- **UI**: `@Composable` screens + `ViewModel` per screen, each exposing a single `StateFlow<UiState>` (sealed: `Loading` / `Success(data)` / `Error(message)`).
- **Data**: one `Repository` per feature (`AuthRepository`, `DeviceRepository`) wrapping a Retrofit `ApiService`, mapping HTTP errors to a `Result`-like sealed type.
- **Session**: `AuthRepository` exposes `Flow<AuthState>` (`LoggedOut` / `LoggedIn(user)`) backed by the encrypted token store; the root `NavHost` observes it to pick the start destination and to force-navigate to login on refresh failure.

### Auth & token handling

- Login/register store `access_token` + `refresh_token` in `EncryptedSharedPreferences`.
- An OkHttp `Authenticator` attaches the access token and, on `401`, calls `/auth/refresh` (mutex-guarded so concurrent requests don't all refresh at once), retries once, and on refresh failure clears the session → `AuthState.LoggedOut`.

### Screens / nav graph

1. **Splash** — reads `AuthState` once, routes to Home or Login (no visible UI beyond app icon/branding).
2. **Login** / **Register** — username/email/password forms, calls `/auth/login` or `/auth/register`.
3. **Home (device list)** — cards per device: name, % gas remaining as a circular gauge, last-updated time, low-battery/no-data/not-configured status badge. Pull-to-refresh; polls `/dashboard` every ~30–60s while the screen is visible (`repeatOnLifecycle(STARTED)`), no background sync for MVP.
4. **Add device (QR scan)** — CameraX preview, requests `CAMERA` permission via `rememberLauncherForActivityResult`, ML Kit analyzer parses the `verolic://pair?...` payload, calls `POST /devices/claim`. On success → step 5.
5. **Bottle setup** — tara weight input (numeric, kg) + bottle type picker (from `GET /bottle-types`), calls `PATCH /devices/{id}/config`. Reachable both right after pairing and later from device detail (for correcting a bad entry).
6. **Device detail** — larger gauge, raw weight/battery/temperature, "edit tara/bottle type" entry point into screen 5.

### Security notes

- HTTPS only in production; local dev `baseUrl` via `BuildConfig` per build type, with a `network_security_config.xml` cleartext exception scoped to the dev host only (not wide open).
- Standard password field treatment (`PasswordVisualTransformation`, autofill hints).
- Pairing secret from the QR code is used once to claim and then never displayed or logged again client-side.

### Implementation phases

1. **Setup** — add the dependencies above, Hilt entry point, `BuildConfig` base URL, network security config.
2. **Auth** — API models, `AuthRepository`, encrypted token store, refresh-on-401 authenticator, login/register screens, session-aware `NavHost`.
3. **Device list & detail** — `DeviceRepository`, home screen with polling + pull-to-refresh, detail screen, `not_configured` handling.
4. **Pairing** — camera permission flow, QR scanner screen, `/devices/claim` integration, bottle-setup screen (tara + bottle type).
5. **Polish** — error/empty/loading states, dark theme check, accessibility (content descriptions, dynamic type), launcher icon/branding.

### Future enhancements (not in scope for MVP)

- Push notification (FCM) or WorkManager-based background check for low-gas alerts.
- Historical usage chart using `/devices/{id}/aggregates`.
- Multiple bottles per household grouping / home-screen widget.