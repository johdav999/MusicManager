# Artist Audition Left Panel Reference Workflow

## Widget

- C++ class: `UAuditionWidget`
- Blueprint asset: `/Game/GUI/Audition/ArtistAuditionPanelBP`
- Parent screen: `LayoutBP1` / `ULayout`
- Reference image: `docs/design/references/artist_audition_left_panel_reference.png`

## User-Facing Outcome

The left panel presents the current unsigned artist audition as a premium label-management card: artist identity, venue, performance attributes, contract offer sliders, and sign/pass decisions.

## Real Data

Runtime values come from `UAuditionWidget::AuditionData`:

- `ArtistData.ArtistName`
- `ArtistData.Genre`
- `VenueName`
- `City`
- `PerformanceScore`
- `StagePresence`
- `AudienceEngagement`
- `VocalQuality`
- `SongwritingQuality`
- `DealData.SignUpBonus`
- `DealData.NumOfRecords`
- `DealData.RoyaltyRate`
- `DealData.ContractYears`

Artist portraits load from `ArtistData.ImageAssetRef` when present. If no image reference exists, the widget uses the generated neutral audition portrait as a production fallback.

## Blueprint Contract

`ArtistAuditionPanelBP` must inherit from `UAuditionWidget` and include these bound components:

- `PanelBackgroundImage`
- `ArtistPortraitImage`
- `VinylFrameImage`
- `TextArtistName`
- `TextGenre`
- `TextVenue`
- `TextCity`
- `TextPerformanceScore`
- `TextStagePresence`
- `TextAudienceEngagement`
- `TextVocalQuality`
- `TextSongwritingQuality`
- `PerformanceMeter`
- `StagePresenceMeter`
- `AudienceEngagementMeter`
- `VocalQualityMeter`
- `SongwritingQualityMeter`
- `SliderSignUpBonus`
- `TextSignUpBonusValue`
- `SliderNumOfRecords`
- `TextNumOfRecordsValue`
- `SliderRoyaltyRate`
- `TextRoyaltyRateValue`
- `SliderContractYears`
- `TextContractYearsValue`
- `ButtonSignArtist`
- `ButtonPass`

## Generated Assets

- `Content/GUI/Audition/ArtistAuditionPanelSurface.png`
- `Content/GUI/Audition/ArtistAuditionContractSurface.png`
- `Content/GUI/Audition/ArtistAuditionVinylFrame.png`
- `Content/GUI/Audition/ArtistAuditionDefaultPortrait.png`
- `Content/GUI/Audition/ArtistAuditionSignButtonSurface.png`
- `Content/GUI/Audition/ArtistAuditionPassButtonSurface.png`
- `Content/GUI/Audition/ArtistAuditionIcon_*.png`

## Runtime Behavior

`ULayout::ShowAuditionWidget()` obtains the next unsigned artist from `UArtistManagerSubsystem`, populates `UAuditionWidget`, and routes the display through `UUIManagerSubsystem`.

`ButtonSignArtist` and `ButtonPass` continue to dispatch `FSignArtistCommand` and `FRejectArtistCommand` through `UCommandDispatcherSubsystem`.
