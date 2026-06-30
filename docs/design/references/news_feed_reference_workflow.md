# News Feed Reference Workflow

## Widget

- C++ class: `UNewsFeedList`
- Item C++ class: `UNewsFeedItemWidget`
- List Blueprint: `/Game/GUI/NewFeedListBP`
- Item Blueprint: `/Game/GUI/NewsFeedItemBP`
- Reference image: `docs/design/references/news_feed_reference.png`

## Reference Breakdown

The news feed is a right-side vertical HUD panel with a matte black/vinyl material, thin gold frame, compact uppercase header, dynamic stacked news cards, and a bottom `VIEW ALL NEWS` command button.

News items must not be baked into the list background. The list Blueprint contains the panel shell, scrollbox, empty `FeedContainer`, hover canvas, and bottom button. Each runtime news event creates a separate `NewsFeedItemBP` card and inserts it into `FeedContainer`.

## Generated Assets

- `Content/GUI/News/NewsFeedPanelSurface.png`: clean panel shell, no hardcoded news items.
- `Content/GUI/News/NewsFeedCardSurface.png`: reusable single-card surface for runtime item widgets.
- `Content/GUI/News/NewsFeedButtonSurface.png`: bottom action button surface.
- `Content/GUI/News/NewsFeedIcon_Microphone.png`
- `Content/GUI/News/NewsFeedIcon_Radio.png`
- `Content/GUI/News/NewsFeedIcon_Handshake.png`
- `Content/GUI/News/NewsFeedIcon_Chart.png`
- `Content/GUI/News/NewsFeedIcon_Calendar.png`
- `Content/GUI/News/NewsFeedIcon_List.png`
- `Content/GUI/News/NewsFeedIcon_ChevronRight.png`

## Blueprint Contract

`NewFeedListBP` must contain:

- `RootCanvas`
- `PanelBackgroundImage`
- `HeaderText`
- `FeedScrollBox`
- `FeedContainer`
- `HoverCanvas`
- `ViewAllNewsButton`
- `ViewAllNewsIcon`
- `ViewAllNewsText`
- `ViewAllNewsChevron`

`NewsFeedItemBP` must contain:

- `CardBackgroundImage`
- `NewsTypeIcon`
- `AccentDivider`
- `HeadlineText`
- `SourceText`
- `DateIcon`
- `DateText`

## Runtime Behavior

`UNewsFeedList::AddNewsCard` creates `NewsFeedItemWidgetClass`, calls `SetupFromEvent`, inserts it at the top of `FeedContainer`, then broadcasts `OnNewsFeedCardAdded` and `On News Card Added`.

`FeedContainer` is intentionally cleared on construct by default so any designer preview card cannot become a gameplay-start hardcoded item.
