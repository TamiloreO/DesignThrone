# TestFlight pipeline

The app is built, signed, and uploaded by GitHub Actions, so a Mac is never
needed. `.github/workflows/testflight.yml` archives a Release build on a macOS
runner, signs it with an App Store Connect API key, and uploads it straight to
App Store Connect. TestFlight then installs it on your phone.

The workflow is manual: Actions tab, TestFlight, Run workflow. Pushing a `v*` tag
does the same thing.

## What you have to do once

Everything below is account setup. None of it needs a Mac.

1. **Join the Apple Developer Program.** 99 USD a year, individual or
   organisation. Nothing can be signed or uploaded without it. Enrolment can take
   a day or two, and Apple sometimes asks for identity verification.

2. **Accept the agreements.** App Store Connect, Business, check that the free
   apps agreement is in place. Uploads fail with a confusing error when an
   agreement is pending.

3. **Create the app record.** App Store Connect, My Apps, plus button, New App.
   Platform iOS, name of your choice, primary language, bundle id
   `com.tamiloreo.videoeditor`, SKU anything unique. If the bundle id is not in
   the list yet, register it first under Certificates, Identifiers and Profiles,
   Identifiers, plus button, App IDs, App.

4. **Create an App Store Connect API key.** Users and Access, Integrations, App
   Store Connect API, Team Keys, generate. Give it the Admin or App Manager role,
   because the pipeline uses it to create signing certificates and profiles.
   Download the `.p8` file: Apple lets you download it once. Note the Key ID and
   the Issuer ID shown on the same page.

5. **Add four repository secrets.** Repository, Settings, Secrets and variables,
   Actions, New repository secret:

   | Secret | Value |
   | --- | --- |
   | `APPLE_TEAM_ID` | Ten character team id, from Membership details |
   | `ASC_KEY_ID` | Key ID of the API key |
   | `ASC_ISSUER_ID` | Issuer ID from the same page |
   | `ASC_KEY_P8` | The whole `.p8` file, pasted as text |

   For `ASC_KEY_P8`, open the file in any text editor and paste everything,
   including the `-----BEGIN PRIVATE KEY-----` lines. If pasting a multi-line
   secret is awkward, base64 the file instead and paste the single line; the
   workflow detects which one it was given.

6. **Run it.** Actions, TestFlight, Run workflow. The version defaults to `0.1.0`
   and the build number defaults to the run number, which only ever goes up.

7. **Install it.** When the run finishes, the build spends 5 to 15 minutes
   processing in App Store Connect. Then open TestFlight, Internal Testing, add
   yourself as a tester, and install the build from the TestFlight app on your
   phone.

## What the pipeline does

1. Checks that all four secrets exist, and fails with a named message if one is
   missing.
2. Selects Xcode 26.3 on the runner. The image default is older, and App Store
   uploads have required the iOS 26 SDK since April 2026.
3. Generates the Xcode project with XcodeGen, then archives for a generic iOS
   device in Release.
4. Signs with automatic signing, using the API key to fetch or create the
   distribution certificate and profile. No `.p12` or provisioning profile is
   stored anywhere.
5. Exports with `method=app-store-connect` and `destination=upload`, so the build
   goes to App Store Connect without an intermediate file.
6. Writes the version and build number into the job summary.

## Costs

This repository is private, so macOS runner minutes bill at ten times the Linux
rate. One archive and upload is roughly 6 to 10 minutes of macOS time.

## When it fails

- **`No profiles for 'com.tamiloreo.videoeditor' were found`**: the bundle id is
  not registered, or the API key is not Admin or App Manager. Register the App ID
  and regenerate the key with the right role.
- **`The app record could not be found`**: step 3 is missing. Uploading needs the
  app to exist in App Store Connect before the first build arrives.
- **`ITMS-90062: build number must be higher`**: the build number was reused.
  Leave the input empty so the run number is used, or pass a higher one.
- **`Missing required icon file`**: the asset catalog is committed at
  `ios/VideoEditor/Assets.xcassets`; if that error appears, check that
  `ASSETCATALOG_COMPILER_APPICON_NAME` is still `AppIcon` in `ios/project.yml`.
- **Agreement or tax errors**: step 2.

## Internal versus external testing

Internal testing allows up to 100 members of your team and needs no review, which
is the right setting while the app is still being built. External testing allows
up to 10,000 testers but requires a Beta App Review and a privacy policy URL, so
it is a later step, not the first one.

## What this does not do

- It does not create the App Store listing, screenshots, or the privacy answers.
  Those are needed for review, not for TestFlight.
- It does not run on every push. A build that testers get should be a deliberate
  action.
