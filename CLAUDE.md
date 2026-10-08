# Notes for working in this repo

## Branch

- **All work goes on `my-mods`.** Commit and push there (`git push origin my-mods`).
- Never commit to or push `master`, `2025q3` or any other branch, and don't open
  PRs against them unless explicitly asked. `master` / `2025q3` track Keychron's
  upstream; `my-mods` carries the personal firmware.

## Layout

- Shared user code: `users/simon/` (`simon.c`, `host_protocol.{c,h}` for
  kbd-daemon, `openrgb.{c,h}` for the OpenRGB protocol).
- Keymaps: `keyboards/keychron/q5_he/ansi_encoder/keymaps/mine` and
  `keyboards/keychron/q1v2/ansi_encoder/keymaps/mine`. Both use `users/simon`,
  so build both after changing it:

  ```bash
  qmk compile -kb keychron/q5_he/ansi_encoder -km mine
  qmk compile -kb keychron/q1v2/ansi_encoder -km mine
  ```

- The host side (kbd-daemon, openrgb-daemon, qmk-host) lives in
  `~/Projects/github/openrgb_client` (branch `main`); `host_protocol.h` is the
  reference for the packet layout both sides share.
