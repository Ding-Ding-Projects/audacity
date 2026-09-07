# App-logo customization backend

This backend supports a presentation-only local logo choice. It does not alter the executable name, executable icon, installer identity, updater feed, package metadata, or release identity.

## Storage and input contract

`au::branding::BrandingStore` receives an explicit profile-root directory in its constructor. It has no global application-data fallback. The store accepts byte payloads only, so source filenames and source paths never enter persistence. The cache is versioned (`branding-v1`) and contains a validated source blob, metadata, and deterministic PNG derivatives at 16, 32, 48, 64, 128, and 256 pixels.

The bounded input limit is 8 MiB with a maximum raster dimension of 4096 by 4096 pixels. PNG, JPEG, WebP, ICO, and SVG are accepted only when Qt can decode them. SVG input is rejected before decoding if it contains scripts, external references, embedded image elements, stylesheet URL references, imports, or document/entity declarations. The backend does no network operation.

## Rendering and recovery

The caller selects `Fit` or `Crop` and a background colour. Derivatives are rendered in memory, then written into a staging cache with atomic file writes. The staging cache is activated only after every derivative and metadata file succeeds. Decode, write, and cancellation failures retain the prior active state. Reset removes only this local presentation cache and restores the shipped mark selected by the owning UI.

## Integrated personalization surface

The Personalize preferences page supplies the shipped mark bytes, derives its profile root from the injected global configuration, calls `loadCustom`, exposes fit/crop and background controls through `update`, renders the generated preview, and resets the cache. The selected file path is read once and is never persisted. The status text reports saved, reset, and rejected states locally.

The shared Material title bar consumes the live 32 px variant and immediately returns to the shipped-mark fallback after reset. `BrandingModel` exposes 16, 32, 48, and 64 px properties for other existing presentation consumers. Installer, executable, updater, package, and release metadata continue using the shipped identity. A platform surface that has no rendered mark is intentionally left unchanged.

## Verification

The standalone Qt/MSVC test target at `src/branding/tests` covers malformed and oversized input, crop and fit pixels, alpha and background handling, persistence/reset, cancellation and failed replacement preservation, and unsafe SVG rejection. The integration model uses the same bytes-only backend and performs no network operation.
