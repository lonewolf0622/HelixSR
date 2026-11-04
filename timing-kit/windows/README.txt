FSR 4.1.1 timing kit for Windows (EXPERIMENTAL)
===============================================

Experimental: reports from Windows PCs are in from more than twenty graphics chips so far. If it
fails or shows something odd on your PC, that is exactly the report the project needs.

Times FSR 4.1.1 the way a game runs it, with AMD's own shaders and with this project's DLLs, on
your graphics card. No game and no OptiScaler are needed, and nothing is installed or changed.
https://github.com/zgauthier2000/fsr4-shader-performance-overrides

How to run it
  1. Unpack the whole folder somewhere (not inside the zip viewer). The DLLs come packed in
     dlls.pack.zst: the program takes out each one when it times it and replaces it with the next,
     so only one DLL (about 40 MB) is on disk at a time, and none when it has finished.
  2. Close games, browsers and video players.
  3. Double-click fsr4time.exe. It takes about 15 minutes, 17 on Radeon RX 7000 cards and RDNA3 integrated
     graphics, which also time the three pass 11 test DLLs below, and about 21 on Radeon RX 6000 cards and RDNA2
     integrated graphics, which also time those and the five packing test DLLs (a slow or integrated graphics
     chip up to about 25-30). Leave the PC alone while it runs: the header at the top of the window says so the whole time.
  4. At the end it shows a summary, saves it as results-<time>.txt in this folder, and asks
     whether to send it to the project. Only two files are sent, in one short message: the summary
     you see, and results-raw.txt (in the same folder, so you can read it first): each run's time
     and picture fingerprint, from which the summary is made. If you send it, you can also type
     a name or nickname for the project's thank-you page; that is optional, and it is published
     there only if you type one. Each summary has a report ID (eight letters and digits); if you
     write to the project about a run, please mention it.

The study run (RDNA2 and RDNA3 graphics only, about 30 minutes)
  run-study.cmd starts a longer test that times each change in the exact DLL on its own, to find
  which help on your kind of chip. The DLLs for it are in the same pack; of them, the normal test
  above times only the three pass 11 test DLLs of your kind of card and, on RX 6000 and RDNA2
  integrated graphics, the five packing test DLLs. See STUDY.txt.

Windows will warn about an unknown program, because the file is not signed. The source is in
the project's repository (timing-kit/windows/fsr4time.c) with the command that builds it, and the
checksums are on the download page.

What it does
  For every folder in dlls\ it loads that upscaler DLL through AMD's FidelityFX loader, feeds it
  made-up frames at 1080p, 1440p and 4K output, with two render sizes: the Balanced preset's, and
  Ultra Performance's (a third of the output, marked "UP" in the summary; the UP lines are for that
  mode only and do not describe the lossy DLL in other modes). Each with the picture
  still and with the camera moving. It measures the graphics card's time for each upscale call:
  200 calls after 40 to warm up.

  It does this in rounds: all DLLs in name order, then all in reverse order, and so on, for about
  15 minutes (more with the test DLLs of your kind of card, so each DLL gets as many rounds). A fast card gets up to eight rounds, a slow one two (one, if a single round takes
  more than ten minutes). More rounds make the figures steadier, and a reading disturbed by
  something else on the PC shows up as a spread between a DLL's runs. Time left over after the
  rounds goes to the DLLs whose runs disagreed by more than 3%: they are timed again. It also
  takes a fingerprint of the picture each DLL produces (its last frame), to show which DLLs give
  the same picture.
  Where a DLL's picture is not the same as AMD's, the summary also says how far it is from it:
  in dB (higher is closer), the share of values more than one step of 255 off on screen, and
  the largest difference. "identical on screen" means the difference is only in rounding that a
  screen does not show. While it runs, the kit keeps AMD's last pictures in pic-*.tmp files here
  (up to a few hundred MB) and deletes them at the end.

  Each DLL runs in a program of its own. If one crashes, or does not finish in time (a driver
  that stops responding), the kit notes it in the summary and goes on with the next one. The time
  limit is three times the longest DLL run that finished on your PC, at least 2 and at most 10
  minutes, so a stuck DLL usually costs only a few minutes.

  dlls\1-amd-shaders   AMD's DLL with AMD's shaders (only its graphics-card check is lifted, so
                       that it runs on RX 6000 cards and integrated graphics too): the reference
  dlls\1b-amd-shaders-dots-split
                       the same, with one change to the last shader: its dot products are split
                       into two steps. The result is the same number, but RX 6000 cards and the
                       RDNA2 graphics built into some processors (Steam Deck, Radeon 680M, 660M,
                       610M, the graphics of Ryzen 7000 and 9000 desktop processors) are known to
                       show ghosting under Windows with AMD's original form. On those chips this is
                       the DLL to compare with, and the summary's percentages are against it.
  dlls\n-0-reference, n-exact-r3, n-exact-r2, n-exact-igpu-r3, n-exact-igpu-r2,
  n-lossy-r3, n-lossy-r2, n-lossy-igpu-r3, n-lossy-igpu-r2
                       unreleased test builds of this project's exact and lossy DLLs and a reference
                       build: r3 is for Radeon RX 7000 cards, r2 for RX 6000 cards, igpu-r3 for RDNA 3 integrated graphics (Radeon
                       780M, 890M and others) and for GeForce, Arc and other cards, igpu-r2 for the Steam
                       Deck, Radeon 680M and older integrated graphics. Each n-exact should give n-0's
                       picture exactly; the n-lossy ones give one picture between them, different from
                       n-0's by design. They are compared with each other, not with AMD's picture.
                       Every PC times all nine, so the summary shows how each kind of build does on your card.
  dlls\n-exact-r3-alt, n-exact-r2-alt, n-exact-igpu-r3-alt, n-exact-igpu-r2-alt,
  n-lossy-r3-alt, n-lossy-r2-alt, n-lossy-igpu-r3-alt, n-lossy-igpu-r2-alt,
  n-exact-r3-alt2, n-exact-r2-alt2, n-exact-igpu-r3-alt2, n-exact-igpu-r2-alt2
                       each the n-exact or n-lossy build of the same name with one shader (the first one)
                       written in a different way; timed on every PC and compared with that build (the
                       summary names it beside it). The alt2 builds should give their n-exact build's picture
                       exactly. They are compared with each other, not with AMD's picture.
  dlls\lt1-reference, lt2-test, lt3-test, lt4-test, lt5-test, lt6-test, lt7-test, lt8-test,
  lt9-test, lt10-test, lt11-test, lt12-test
                       a reference build and eleven test builds of a smaller FSR 4 network, timed on every PC:
                       lt2 should give lt1's picture exactly (the summary names lt1 beside it), lt3 is a
                       faster form whose picture differs from lt1's by design, and lt4 and lt5 are lt3 with
                       one small change each. lt6 is lt2 with its first shader written in a different way and
                       should give lt2's picture exactly; lt7 is lt3 with lt4's and lt5's changes, two of them
                       written more compactly, and the same first shader as lt6; lt8 is lt7 without those two
                       rewrites and should give lt7's picture exactly (both differ from lt3's); lt9 is a lighter
                       form of lt7 (lt3 with lt6's first shader), timed against lt7; its picture differs from lt7's.
                       lt10 is lt3 without the two shader changes lt2 makes and should give lt3's picture
                       exactly; lt11 and lt12 are lt1 with one of those two changes each and should give lt1's
                       picture exactly.
                       They are compared with each other, not with AMD's picture.
  dlls\rdna2-f1-exact, rdna2-t1-tiled-pass11, rdna2-t2-amd-cut-pass11 (RX 6000 and RDNA2 integrated graphics only)
  dlls\rdna3-f1-exact, rdna3-t1-tiled-pass11, rdna3-t2-amd-cut-pass11 (RX 7000 and RDNA3 integrated graphics only)
                       test DLLs for the second-to-last network shader: f1 is a test build of the next release's
                       exact DLL for that kind of card; t1 is f1 with that shader working on small square tiles
                       of the picture instead of rows, t2 the same shader cut into tiles the way AMD's newer
                       RX 6000 driver shaders do it. Only the speed may differ between the three.
  dlls\rdna2-p1-amd-shaders, rdna2-p2-dots-split, rdna2-p5-exact, rdna2-p6-swap, rdna2-p7-swap-handpack,
  rdna2-p8-exact-swap, rdna2-p9-alt-shaders
                       (RX 6000 and RDNA2 integrated graphics only) the packing test: which way of writing the
                       last shader gives the RX 6000 picture under Windows its difference from AMD's; p6 and p7
                       are compared with p2, p8 with p5, p9 with p1 (details in STUDY.txt).
  The test DLLs and reference builds are timed for their speed and compared with each other, not
  with AMD's picture: a large difference from AMD's picture on their lines is expected.

  To time another FSR 4.1.1 DLL as well, and to see whether it gives the same picture as one of
  these, make a folder dlls\ next to fsr4time.exe, a folder for the DLL in it, and put its amd_fidelityfx_upscaler_dx12.dll there. For
  example, if you have the community's 4.1.1b file itself (the kit does not include it), put it
  in dlls\7-411b. The summary then has a line for it at every size, with its time and with
  which of the DLLs above, if any, gives the same picture.

  The DLL does not have to be FSR 4.1.1: any upscaler DLL that offers an FSR 4 version on your
  card is timed, and one that offers several (variants of the model for different kinds of
  graphics chip, for instance) gets a block of lines for each of them.

Reading the summary
  The numbers are milliseconds of graphics-card time per upscaled frame; lower is better. Each
  is the mean of that DLL's runs (one per round, and any run it got when timed again), and where
  they spread by more than 3% the smallest and largest are given in square brackets: treat such a
  line with caution. The percentage compares with the first DLL, or on an RDNA2 chip (RX 6000 or
  the built-in ones listed above) with 1b-amd-shaders-dots-split. For a lossy DLL the two numbers in brackets are a frame that runs
  FSR 4's model and a frame that skips it.

  After each run comes the picture's fingerprint. "= 1-amd-shaders" after it means the
  output was the same as that DLL's, byte for byte; "(differs from those above)" means no DLL
  before it gave that picture. A lossy DLL always differs, as it should. On an RDNA2 chip under
  Windows the exact DLLs may differ from 1-amd-shaders (earlier kits found that); that is worth
  sending too.

  A DLL should give the same picture in all of its runs. Where it does not, the line says
  "NOT the same picture in all its runs" and, since kit 2026-10-12.3, how different: how many
  16x16-pixel blocks of the picture differ, the area they lie in, and the largest shift of a
  block's mean value. That is a finding worth sending.

  These are made-up frames. A game's numbers will differ somewhat; what OptiScaler's overlay
  shows in a game is the real thing, and the kit asks for it at the end if you have it.

Options (from a command prompt in this folder)
  fsr4time.exe --quick           1080p and 4K only
  fsr4time.exe --all-presets     also the Quality and Performance presets' render sizes: twelve sizes
                                 instead of six, so the rounds get about 30 minutes (lines marked "Q"
                                 and "P"; no mark = Balanced)
  fsr4time.exe --once            one round only (a few minutes)
  fsr4time.exe --minutes 8       let the rounds take about 8 minutes instead of 15
  fsr4time.exe --no-questions    no questions at the end, nothing is sent
  fsr4time.exe --study           the study run (what run-study.cmd starts; see STUDY.txt)

Other graphics cards
  The kit is not limited to Radeon. FSR 4.1.1 should run on any graphics chip whose driver supports
  DirectX 12 with Shader Model 6.6. If you have a GeForce, an Intel Arc or Intel integrated
  graphics, or an older Radeon, please run it too:
  these DLLs were made for Radeon chips, nobody has measured them elsewhere yet, and your result
  shows whether they help there and whether the exact DLL still gives AMD's picture.

Windows 10
  Windows 10's own DirectX 12 runtime offers shader model 6.5, and FSR 4 needs 6.6: on such PCs every
  DLL, AMD's own included, crashed inside the runtime. Games bring a newer runtime of their own
  (Microsoft's DirectX 12 Agility SDK); the kit can do the same:
  - when Windows' runtime offers less than shader model 6.6, the kit offers to fetch Microsoft's
    runtime (D3D12Core.dll, a 22 MB download from Microsoft's package on nuget.org) into the folder
    D3D12 here. The runtime is Microsoft's file, under Microsoft's license; the kit does not carry it.
    The kit checks the file, then runs the DLLs with win10-helper.exe, the same program set up to use
    that runtime.
  - by hand: from Microsoft.Direct3D.D3D12 version 1.615.1 on nuget.org, the file
    build/native/bin/x64/D3D12Core.dll goes into a folder D3D12 next to fsr4time.exe.
  - win10-helper.exe is the kit's helper for this: do not start it yourself (opened by hand it only
    says so). It is used only on such PCs; where Windows' runtime offers 6.6 or more (Windows 11),
    nothing changes.
  The summary's GPU line says which DirectX 12 runtime was used ("from the kit" or "built in") and the
  highest shader model the card offers. Reports from Windows 10 are welcome, crashes included.

Not for Radeon RX 9000 cards: they run a different FSR 4.

The DLLs are modified versions of AMD's; see NOTICE.md. The program is licensed under the GNU
GPL v2 or later.
