Game Design: Music Label Manager
1. Core Concept

A music industry simulation and management game where the player runs their own record label from the 1950s to the 2020s.
The player signs artists and bands, records and releases albums, manages marketing and distribution, competes on charts, sends artists on tours, responds to critics, and balances the financial pressures of running a label.

The game blends management strategy with immersive performance experiences powered by Unreal Engine and MetaHumans, where players can actually see artists perform songs.

2. Game Pillars
Authentic Music History Progression: Each decade introduces new genres, trends, and technology (vinyl, cassette, CD, streaming).
Artist & Band Simulation: Manage personalities, careers, and artistic directions.
Song Library: Pre-created songs (using Suno AI) across all genres, tagged with attributes (energy, lyrics, genre, target demographics, mood).
Immersive Performances: Watch concerts, live shows, and music videos in-game using MetaHumans.
Business Management: Studio time, marketing, distribution, financial reports, and competition with rival labels.
3. Time Progression
1950s–1960s: Birth of rock ’n’ roll, jazz, folk, early pop. Radio is king.
1970s–1980s: Disco, punk, metal, hip-hop emerge. Rise of stadium tours. Vinyl → cassettes.
1990s–2000s: Grunge, boy bands, electronic, pop-punk, hip-hop mainstream. CD era → MP3s.
2010s–2020s: Streaming platforms, trap, EDM, indie revival, viral hits. Algorithms drive discovery.
4. Systems & Features
4.1 Artist & Band Creation
Attributes: Talent, Charisma, Genre Affinity, Reliability, Market Appeal.
Personalities: Ego, teamwork, lifestyle choices (scandals, drug use, activism).
Career Path: Can decline if neglected or burn out on tours.
MetaHuman Performances: Player can watch concerts, see music videos, and showcase artists live.
4.2 Song System
Songs are pre-generated with Suno AI per genre. Each song has:
Genre tags (Rock, Pop, Rap, EDM, Jazz, etc.)
Mood (Happy, Angry, Romantic, Melancholic)
Consumer Appeal (Teenagers, Young Adults, Mature Listeners, Niche Fans)
Quality (basic metric, influenced by artist performance in studio)
Songs are used to create albums, singles, or EPs.
Licensing & covers possible later in game.
4.3 Production & Studio System
Rent studio time or build your own.
Invest in producers, sound engineers.
Manage creative direction: “radio-friendly single” vs. “experimental album.”
Studio costs scale with quality of production.
4.4 Marketing & Distribution
Marketing Channels evolve:
1950s: Radio, posters, newspapers.
1980s: MTV, magazines, TV shows.
2000s: Internet, MySpace, blogs.
2010s+: TikTok, Spotify playlists, influencers.
Budgets: Can push singles heavily or let word of mouth grow.
Distribution: Vinyl, cassette, CD, streaming.
4.5 Top Lists & Critics
Weekly charts by sales/streams.
Critics give reviews that influence reputation.
Awards ceremonies (Grammys, MTV Awards, etc.) boost credibility.
4.6 Tours & Concerts
Design tours (venues, countries, scale).
Risk vs reward: Big stadium tour can bankrupt if ticket sales flop.
Live performance (Unreal + MetaHumans): Player watches the show with generated stage effects, fans, energy.
4.7 Finance & Management
Income: Record sales, tours, merchandise, licensing.
Costs: Studio, salaries, marketing, distribution, legal fees.
Rival labels compete for artists and chart dominance.
Bank loans or investors if struggling financially.
5. Gameplay Loop
Discover & Sign Artists
Scout talent (local clubs, festivals, viral hits online).
Negotiate contracts (advance, royalties, tour splits).
Produce Music
Select songs from the pre-tagged library.
Book studio sessions.
Release singles and albums strategically.
Promote & Distribute
Choose marketing campaigns.
Push for radio/streaming playlist placement.
Release in global markets (US, Europe, Japan, etc.).
Track Success
Monitor charts, sales, and streaming.
Read critics’ reviews.
Adjust strategy based on feedback.
Tour & Perform
Plan and manage tours.
Watch concerts live with crowd reactions.
Expand & Survive
Grow label reputation.
Expand into multiple genres.
Navigate bankruptcies, changing trends, and artist drama.
6. Game Modes
Campaign Mode: Start in the 1950s and play through decades. Unlock new genres, technologies, and challenges.
Sandbox Mode: Jump into any era with free play.
Challenge Scenarios:
“Save the struggling 80s rock band.”
“Break into the 2000s boy band craze.”
“Build a viral TikTok artist in 2020.”
7. Visual & Audio Style
Visuals: Photorealistic (Unreal 5 + MetaHumans).
Concerts: Full stage lighting, crowds, and animations.
Studios & Offices: Immersive label HQ, recording sessions.
UI: Clean, inspired by record label dashboards & vintage posters evolving with time.
Audio: Hundreds of Suno AI generated tracks per genre, evolving instruments/production styles across decades.
8. Player Experience

Players feel the thrill of discovering the next big star, the pressure of balancing budgets, and the satisfaction of watching their artists dominate charts and stages.
Every decision—from choosing the right single to handling an artist’s meltdown—shapes the legacy of the player’s label.

Architecture
1. Top-level architecture

This is a single Unreal runtime module with most game logic implemented as UGameInstanceSubsystems, and most UI implemented as UUserWidgets. The architectural center is the combination of:

UGameTimeSubsystem: monthly simulation clock and orchestrator (GameTimeSubsystem.cpp:34)
UUIManagerSubsystem: UI routing/orchestration layer (UIManagerSubsystem.cpp:16)
ULayout: root viewport widget holding all major panels (Layout.cpp:29)
The main pattern is:
PlayerController -> Layout -> UI widgets
and
GameTimeSubsystem -> simulation subsystems -> UIManager/Layout -> widgets.

2. Runtime boot flow

AMusicManagerGameMode sets the default pawn and controller (MusicManagerGameMode.cpp:6).
AMusicManagerPlayerController::BeginPlay() creates a ULayout widget and adds it to the viewport (MusicManagerPlayerController.cpp:14).
ULayout::NativeConstruct() registers itself with UUIManagerSubsystem, initializes layer visibility, canvas state, region map, and artist panel bindings (Layout.cpp:29).
UUIManagerSubsystem::Initialize() subscribes to UEventSubsystem news generation and UArtistManagerSubsystem artist change delegates (UIManagerSubsystem.cpp:16).
3. Core simulation subsystems

UGameTimeSubsystem
State: CurrentGameDate, bIsTimeRunning, TimeAdvanceHandle, bHasReachedSimulationEnd.
Role: advances one month every 4 seconds, then explicitly calls Market, Artist, Record, and Finance subsystems in order, and finally broadcasts OnMonthAdvanced (GameTimeSubsystem.h, GameTimeSubsystem.cpp:34).

UArtistManagerSubsystem
State: ActiveContracts, ExpiredContracts, UnsignedArtists, ArtistToSongs, SelectedArtistId, ArtistMomentum, ArtistReputation, ConcurrentReleasesCache, ArtistActionAvailability.
Role: artist roster, signing/rejecting, contract lifecycle, selected artist context, per-artist sales modifiers, action availability.
Key functions: SignArtist, HandleMonthAdvanced, GetArtistMarketModifiers, UpdateArtistActionAvailability, SetSelectedArtist (ArtistManagerSubsystem.cpp:392, ArtistManagerSubsystem.cpp:470, ArtistManagerSubsystem.cpp:716).

USongManagerSubsystem
State: SongDataTable, Songs, SongMap, LockedSongIds, SongToRecordMap.
Role: song registry/factory, per-artist lookups, locking songs during recording, save serialization.
Key functions: CreateSong, GetEligibleSongsForRecording, LockSongsForRecording, SerializeForSave (SongManagerSubsystem.cpp:58, SongManagerSubsystem.cpp:191, SongManagerSubsystem.cpp:232).

URecordManagerSubsystem
State: Records, SalesHistory, LifetimeUnits, FormatRules, RecordStates, ActiveRecordingIntents, RecordingStartDates, RecordingCompletionDates.
Role: recording workflow, release creation, lifecycle state, monthly sales simulation, finance handoff.
Key functions: SubmitRecordingIntent, CompleteRecording, SimulateMonthlySales (RecordManagerSubsystem.cpp:114, RecordManagerSubsystem.cpp:395, RecordManagerSubsystem.cpp:190).

UMarketManagerSubsystem
State: LoadedRegions, LoadedSegmentProfiles, RegionSegments, RegionArtistExposure, RegionRecordExposure.
Role: market data loading, segment resolution, demand snapshot building, radio exposure simulation, momentum boost generation (MarketManagerSubsystem.cpp:7, MarketManagerSubsystem.cpp:132, MarketManagerSubsystem.cpp:307).

UFinanceManagerSubsystem
State: LabelAccounts map of FLabelAccount { LabelId, CurrentBalance, Ledger }.
Role: ledger and cash balance bookkeeping, monthly profit reporting, applying record sales revenue/cost/royalty entries (FinanceManagerSubsystem.cpp:4, FinanceManagerSubsystem.cpp:95).

UEventSubsystem
State: ProcessedNewsKeys, cached GameTimeSubsystem, optional layout ref.
Role: listens to month advancement, generates deduplicated FMusicNewsEvent, forwards it to UI (EventSubsystem.cpp:14, EventSubsystem.cpp:136, EventSubsystem.cpp:199).

UMusicSaveSubsystem / UMusicSaveGame
Saves songs, contracts, date; reloads those and asks UUIManagerSubsystem to rebuild UI afterward (MusicSaveSubsystem.cpp:13, MusicSaveSubsystem.cpp:66, MusicSaveGame.h).

4. GUI architecture and subsystem integration

ULayout is the composition root for GUI. It owns references to:

UInspectorPanelWidget
UMainCanvasHost
UHoverTooltipManagerWidget
UArtistHoverDetailWidget
UNewsFeedList
UAuditionWidget
UContractWidget
URecordWidget
USignedArtistPanelWidget
URegionMapWidget
That means widgets generally do not create top-level widgets themselves. They signal intent upward; ULayout and especially UUIManagerSubsystem decide what to show.

Important linkage paths:

Signed artist click:
USignedArtistItemWidget::HandleClicked() -> USignedArtistPanelWidget::OnArtistSelected -> ULayout::HandleArtistSelected() -> UArtistManagerSubsystem::SetSelectedArtist() + UUIManagerSubsystem::SetSelectedEntity() (SignedArtistItemWidget.cpp:182, SignedArtistPanelWidget.cpp:103, Layout.cpp:525, UIManagerSubsystem.cpp:460).

Signed artist hover:
USignedArtistItemWidget::HandleHovered() -> UUIManagerSubsystem::ShowArtistHover() -> ULayout::ShowArtistHoverDetail() (SignedArtistItemWidget.cpp:201, UIManagerSubsystem.h, Layout.h).

Command panel:
UCommandPanelWidget spawns UCommandItemWidget children, receives click callbacks, and forwards command strings to UUIManagerSubsystem::HandleCommandAction() (CommandPanelWidget.cpp:61, CommandPanelWidget.cpp:148).

Record flow:
ULayout::ShowRecordWidget() chooses the selected artist and initializes URecordWidget;
URecordWidget::OnConfirmPressed() builds FRecordRecordingIntent;
URecordManagerSubsystem::SubmitRecordingIntent() validates, locks songs, and starts the recording;
completion updates song state and broadcasts OnArtistRecordCreated, which UArtistManagerSubsystem uses to refresh action availability (Layout.cpp:360, RecordWidget.cpp:95, RecordManagerSubsystem.cpp:114).

News flow:
UGameTimeSubsystem advances month -> UEventSubsystem generates FMusicNewsEvent -> UUIManagerSubsystem::HandleNewsEvent() -> ULayout::AddNewsCardToFeed() -> UNewsFeedList::AddNewsCard() -> UEventTickerWidget hover/click behavior (EventSubsystem.cpp:136, NewsFeedList.cpp:79, EventTickerWidget.cpp:178).

Audition flow:
clicking a “new upcoming artist” news ticker calls Layout->ShowAuditionWidget() and also starts AAuditionEventActor::StartAudition(); that actor owns UMusicPlayerComponent, which registers into UUIManagerSubsystem so song previews can be played from UI widgets too (Layout.cpp:294, AuditionEventActor.cpp:34, MusicPlayerComponent.cpp:59).

5. Main GUI classes

UMainCanvasHost: holds canvas state and optional layer-3 modal/screen widget. Pure host, no spawning outside assignment from UUIManagerSubsystem (MainCanvasHost.cpp:17).
UInspectorPanelWidget: persistent read-only inspector for selected entity; calls IInspectable::PopulateInspector() if implemented (InspectorPanelWidget.cpp:6).
USignedArtistPanelWidget: scroll list of signed artists, maintains SelectedArtistId, rebuilds child items on roster changes (SignedArtistPanelWidget.cpp:28).
USignedArtistItemWidget: per-artist portrait item; variables are mostly visual state (FrameMID, color presets, hover/selection flags, cached availability); subscribes to ArtistManagerSubsystem availability events.
UCommandPanelWidget + UCommandItemWidget: command strip and command buttons.
URecordWidget + URecordSongListItemWidget: recording UI, song pick lists, confirm/cancel, playback preview.
UNewsFeedList, UNewsFeedItemWidget, UEventTickerWidget: compact list item + larger hover/detail ticker card.
UStatusWidget: binds to time and current label, pulls values from UFinanceManagerSubsystem (StatusWidget.cpp:9, StatusWidget.cpp:90).
URegionMapWidget + URegionMapButton: region selection UI; currently mostly button registration, not yet tightly connected to market simulation.
UHoverTooltipManagerWidget and UArtistHoverDetailWidget: layer-2 informational hover presenters.
6. Data classes / structs

The main data containers are:

FSongData for song metrics and release/audio fields (FSongData.h)
FArtistData, FDealData, FAuditionEvent (AuditionTypes.h)
FArtistDealTerms, FArtistContract (FArtistDealTerms.h, FArtistContract.h)
FRecordRecordingIntent, FRecordData, FRecordSalesEntry, FRecordFormatRule (RecordManagerSubsystem.h)
FMarketRegion, FMarketSegmentProfile, FMarketDemandSnapshot (MarketRegion.h, MarketManagerSubsystem.h)
FCashFlowEntry, FLabelAccount (FinanceManagerSubsystem.h)
FMusicNewsEvent (EventSubsystem.h)
7. Architectural assessment

Strong points:

subsystem boundaries are clear;
UUIManagerSubsystem is doing real orchestration rather than letting widgets directly own global flow;
monthly simulation order is explicit and deterministic;
widget code usually defers business rules to subsystems.
Current weak points:

some IDs use ArtistName and some use ArtistId; this is a real consistency risk;
several flows rely on “selected artist” global state;
some UI and simulation code still overlap a bit, especially around auditions and news ticker actions;
save/load currently omits several subsystem states (records, finance, market exposure);
IInspectable exists, but I did not find concrete runtime classes implementing it, so inspector integration is only partially realized;
RegionMapWidget appears only lightly connected to market state beyond visibility and selection.
If you want the next step, I can turn this into a stricter class-by-class inventory table with every function/member listed per file, or I can produce a Mermaid/PlantUML dependency diagram for the full runtime architecture.