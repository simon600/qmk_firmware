# Simon's Keychron Q5 HE Keymap & HE Profiles

## Profile Switching Shortcuts

There are two primary ways to switch profiles on your keyboard:

### 1. Dedicated Keymap Cycle Key (`TG_GMG`)
On the **`WIN_BASE`** layer (located at the **F14** position, next to `Delete`):
- **Press 1**: Activates `GAMING` layer + switches to **Profile 1 (Gaming / Rapid Trigger)** & illuminates LED in **Red**.
- **Press 2**: Switches to **Profile 2 (Gamepad Mode)** & illuminates LED in **Green**.
- **Press 3**: Deactivates `GAMING` layer + returns to **Profile 0 (Default / Typing)** & restores standard lighting.

### 2. Built-in Keychron Hardware Shortcut
Hold **`Fn + P`** together, then press:
- **`X`** $\rightarrow$ **Profile 0** (Default / Typing Mode)
- **`C`** $\rightarrow$ **Profile 1** (Gaming / Rapid Trigger Mode)
- **`V`** $\rightarrow$ **Profile 2** (Gamepad / Controller Mode)

---

## Hardcoded Hall Effect (HE) Profiles

### Profile 0 (Default / Typing)
* **Mode**: `AKM_REGULAR` (Static Actuation)
* **Actuation Distance**: Standard $2.0\text{ mm}$ default.

### Profile 1 (Gaming)
* **All standard keys**: Static actuation at **$1.0\text{ mm}$** (`AKM_REGULAR`).
* **WASD keys**: **Rapid Trigger (`AKM_RAPID`)** enabled:
  * Actuation point: $1.0\text{ mm}$
  * Press sensitivity: $0.4\text{ mm}$
  * Release sensitivity: $0.4\text{ mm}$

### Profile 2 (Gamepad / Controller)
* **All standard keys**: Static actuation at **$1.0\text{ mm}$**.
* **Gamepad / Analog bindings** (`AKM_GAMEPAD`):
  * `W` $\rightarrow$ Left Stick Up (`LS_UP`)
  * `A` $\rightarrow$ Left Stick Left (`LS_LEFT`)
  * `S` $\rightarrow$ Left Stick Down (`LS_DOWN`)
  * `D` $\rightarrow$ Left Stick Right (`LS_RGHT`)
  * `U` $\rightarrow$ Right Stick Up (`RS_UP`)
  * `H` $\rightarrow$ Right Stick Left (`RS_LEFT`)
  * `J` $\rightarrow$ Right Stick Down (`RS_DOWN`)
  * `K` $\rightarrow$ Right Stick Right (`RS_RGHT`)
  * `/` $\rightarrow$ Xbox A Button (`XB_A`)
  * `'` $\rightarrow$ Xbox B Button (`XB_B`)
  * `;` $\rightarrow$ Xbox X Button (`XB_X`)
  * `[` $\rightarrow$ Xbox Y Button (`XB_Y`)
  * `Y` $\rightarrow$ Xbox Right Bumper (`XB_RB`)
  * `RCTL` $\rightarrow$ Xbox Right Trigger (`XB_RT`)
  * `RShift` $\rightarrow$ Xbox Right Stick Click (`XB_R3`)
  * `\`` (Grave) $\rightarrow$ Xbox View / Back (`XB_VIEW`)
  * `PgUp` $\rightarrow$ Xbox Menu / Start (`XB_MEMU`)
  * `Home` $\rightarrow$ Xbox Guide (`XB_XBOX`)
  * `Up`, `Down`, `Left`, `Right` $\rightarrow$ D-Pad (`XB_UP`, `XB_DOWN`, `XB_LEFT`, `XB_RGHT`)
