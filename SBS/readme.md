# Satisfactory Blueprint Storage (SBS)

Browse shared blueprints and blueprint packs from the build menu and import them into the current host session without reloading the save.

This development migration targets **Satisfactory 1.2, UE 5.6.1-CSS and SML 3.12.0**. SBS is an active GameFeature under `Mods/GameFeatures/SBS/`, with matching KPrivateCodeLib and KBFL dependencies. Those libraries also require KUI and KAPI.

The backend is being rebuilt at **`https://k-mods.com/api/v1/sbs/`**. The current client retains the existing relative routes and payloads while that service is developed. See [API compatibility contract](Docs/API_CONTRACT.md); production availability and authentication still require verification against the rebuilt backend.

## In-game use

Open the build menu after unlocking blueprints. SBS provides blueprint and pack browsing, filtering, pagination, details and download progress. Downloads install on the host. The maintainer owns the widget wiring for login, uploads and the shared download-ID input. `FSBSUserData.Role` is removed; native account data uses action/service permissions.

## Development and verification

All headers belong in `Source/<Module>/Public/`; all C++ implementations belong in `Private/`. `SBSEditor` contains editor-only migration and validation helpers and is excluded from Shipping.

```powershell
python Scripts/verify_blueprint_contract.py
```

The recorded contract comes from upstream commit `a5da1641aeec7917e07a98f8e2374af962380a7c`. Verification works in a standalone checkout or the combined modding workspace. It checks reflected types, existing functions and properties, pin metadata, delegate signatures, original asset paths and the Public/Private source layout.

After building `FactoryEditor Win64 Development`, run the asset commandlet with the CSS editor:

```powershell
& '<CSS engine>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' '<project>/FactoryGame.uproject' -run=pythonscript "-script=$PWD/Scripts/migrate_assets.py" -unattended -nop4 -NullRHI
```

This validates GameFeature data, compiles all 17 existing Blueprints and runs native parser, memory-limit and file-rollback checks. Reports go to `.validation/`. Widget assets are not saved or migrated by default. The separate, explicit `-SBSMigrateLegacyImageUrls` flag enables the optional replacement of old image URL literals with `GetImageBaseUrl`.

A local fixture supports UI and download testing without production credentials:

```powershell
python Scripts/local_api_fixture.py --sbp '<sample>.sbp' --sbpcfg '<sample>.sbpcfg'
```

Launch a copied test save through Steam with `-SBSLocalTestPort=17891` and a separate `-UserDir=<test-directory>`. Fixture mode uses a dummy account and never reads or sends saved account keys. See [migration evidence and build commands](Docs/MIGRATION.md).

## Credits

Kyri123 / Kyrium — web and mod development. Deantendo — mod icon and UI artwork. Thanks to contributors and patrons for testing.

[Support](https://discord.gg/JsJ9XXWS7Q) · [KMods](https://k-mods.com) · [Patreon](https://www.patreon.com/kmods)
# Account login and blueprint uploads

`USBSSharingSubsystem` exposes browser login, optional Windows Credential Manager storage, local blueprint selection and public multipart upload via `FSBSBlueprintUpload`. UI messages use localizable `FText`. Widget wiring: [Docs/SHARING.md](Docs/SHARING.md). Backend implementation prompt and DTOs: [BACKEND_IMPLEMENTATION_PROMPT.md](BACKEND_IMPLEMENTATION_PROMPT.md).

`CopyDownloadId` produces `SBS-BP:<id>` or `SBS-PACK:<id>`. One input calls `ResolveDownloadId`; `DownloadResolvedId` then queues either kind on the host. See [ID input and permission calls](Docs/DOWNLOAD_IDS.md).

SBS participates in the workspace CI build/release list and Discord release announcements through `satisfactorymoddingtools/Files/config.yml`. Weekly scheduled translation rebuilds retain the existing silent-notification policy. Public source sync includes SBS.
