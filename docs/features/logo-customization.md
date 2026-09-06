# App-logo customization backend

This backend supports a presentation-only local logo choice. It does not alter the executable name, executable icon, installer identity, updater feed, package metadata, or release identity.

## Storage and input contract

`au::branding::BrandingStore` receives an explicit profile-root directory in its constructor. It has no global application-data fallback. The store accepts byte payloads only, so source filenames and source paths never enter persistence. The cache is versioned (`branding-v1`) and contains a validated source blob, metadata, and deterministic PNG derivatives at 16, 32, 48, 64, 128, and 256 pixels.

The bounded input limit is 8 MiB with a maximum raster dimension of 4096 by 4096 pixels. PNG, JPEG, WebP, ICO, and SVG are accepted only when Qt can decode them. SVG input is rejected before decoding if it contains scripts, external references, embedded image elements, stylesheet URL references, imports, or document/entity declarations. The backend does no network operation.

## Rendering and recovery

The caller selects `Fit` or `Crop` and a background colour. Derivatives are rendered in memory, then written into a staging cache with atomic file writes. The staging cache is activated only after every derivative and metadata file succeeds. Decode, write, and cancellation failures retain the prior active state. Reset removes only this local presentation cache and restores the shipped mark selected by the owning UI.

## API and future integration

The owning personalization surface will need to: supply the shipped mark bytes to `presetDefaultMark`, choose an explicit shared-profile root, call `loadCustom`, expose fit/crop and background controls through `update`, render `derivativePaths`, and call `reset`. It must localize and persist its own controls, accessibility labels, history entries, and notifications. This backend intentionally does not register a QML type, alter app chrome, or ship a UI.

## Verification

The standalone Qt/MSVC test target at `src/branding/tests` covers malformed and oversized input, crop and fit pixels, alpha and background handling, persistence/reset, cancellation and failed replacement preservation, and unsafe SVG rejection. Its bytes-only API makes the no-network boundary directly testable without a network fixture.
