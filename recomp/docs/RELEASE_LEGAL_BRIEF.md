# Existing private build: facts for a distribution review

Prepared at szampan's direction on 1 November 2026. This is a technical brief,
not a legal opinion or permission to publish. No material has been sent to EA,
to an attorney or uploaded by this work.

## Proposed distribution

A free PS5 homebrew port of The Sims 2, USA retail Original Xbox release. The
owner would like to share a downloadable native title; each player supplies a
legally obtained ISO. Authored source is GPL-3.0-only, with compatible upstream
licenses/notices retained. This selection does not grant rights in the game.

## Material technical facts

- Candidate native build: f48a3b24684ecd4f2479a7c0, PPSA99145. The faster
  ordered-callback variant changes settings only; executable SHA-256 is
  92a47c8c21296fd153dcaebde5834b5366c4e6930348b232f0767276a24525bd.
- XboxXBERecomp translates the owner XBE's x86 functions to C++; Clang compiles
  these functions to native x86-64 code, linked statically into this eboot.
  Preserving guest behavior is intentional. The binary is not merely a generic
  runtime and does not derive its CPU code from the player's ISO at launch.
- The candidate title folder excludes the original XBE, ISO, extracted game
  assets and personal saves. The ISO is required to install those data locally.
  This data requirement does not remove the translated functions from the eboot.
- The first-run installer verifies the supported executable's SHA-1, copies
  data, patches English notices in a private copy and offers optional explicit
  deletion of the ISO. It does not recompile code on the PS5.
- No Sony SDK, firmware, proprietary runtime implementation or console keys
  are supplied. The owner-prepared homebrew environment is assumed. The public
  platform SDK and RADV rendering stack are separate licensed dependencies.
- The independent game-specific analysis/configuration is generated/reviewed
  against the owner's dump. It does not use the other game's project-specific
  config/hooks/generated code. Generic MIT runtime work is credited; this is
  not a strict clean-room claim.
- Original textures, music and other game resources are not in the source-only
  preview. Authored title artwork is CC0, Kenney prompts CC0, Open Sans OFL-1.1.

## Questions requiring a legal assessment

1. May this particular translated-code binary be publicly distributed under
   applicable law, considering the owner-supplied ISO requirement and relevant
   interoperability/reverse-engineering exceptions? Assess Poland/EU and likely
   international distribution exposure rather than assuming US-only rules.
2. Are any original-game portions or expressions in the binary material to that
   assessment? Does a proposed patch/module split change the assessment?
3. What source/notices and scope statements are required for the GPL-covered
   combination, without purporting to relicense EA's original material?
4. How should title/art/trademark usage and any applicable Xbox game license
   terms be handled? Is explicit rights-holder permission needed for the
   proposed distribution, or is there an applicable statutory basis?

EA's current policy is a relevant publisher statement, not proof of whether a
particular statutory exception applies. It does not expressly authorize this
port and says it does not permit game revivals/fan games:
https://help.ea.com/en/articles/security-and-rules/ea-content-policy/

Small scale, being free, a disclaimer, requiring an ISO, existing GitHub ports
and GPL compliance individually do not establish publication clearance or a
guarantee against a claim. Equally, no definite illegality finding is made here.
No native binary should be described as legally cleared on the strength of this
technical inventory alone.
