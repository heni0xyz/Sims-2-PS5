# PS5-only launcher test candidate 1.1.1

- Plum Glass launcher with Play, Install and Update icons, a subtle gradient on
  the selected tab, and an update notice shown when a new release is available.
- Experimental native Sim portraits. The runtime regenerates supported
  32×32 thumbnails at 128×128 and includes a 512×512 path for Create a Sim.
  Family thumbnails have been checked in the PC renderer. Gameplay, Create a
  Sim and the complete ISO-to-Play flow still need PS5 validation.

# PS5-only launcher source candidate (1.1.0 / 8db5af5a)

- Prepare the owner's USA Xbox ISO on PS5, including code generation,
  compilation, native linking and a shared Play/Install/Update launcher.
- Repair older private executable permissions and hide sections during startup
  inspection. Cached-game boot and bounded shutdowns PASS on hardware.
- New Home-only artwork/global icon, gameplay carousel, navigation/fade/recoil,
  remembered quieter music and installation section lock.
- Add checked public update transactions, complete local SDK/native-writer
  source and repeatable source packaging. Full updater helper acceptance and
  stable gameplay 60 FPS remain open.

This source change does not replace existing GitHub release downloads.

## Published v1.0.0 (legacy PC builder)

Known issues: variable gameplay performance, occasional thin Continue background,
unverified full-game/stability coverage.
