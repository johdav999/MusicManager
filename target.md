# MusicManager Target Feature And GUI List

This document describes the target functionality and GUI coverage for a professional-grade music manager / record label management game.

## Core Game Loop

1. Discover unsigned artists.
2. Evaluate talent, genre fit, market potential, risk, personality, and cost.
3. Negotiate contracts.
4. Develop songs and recordings.
5. Package singles, EPs, albums, deluxe editions, and reissues.
6. Plan release dates, formats, regions, marketing, radio, press, playlisting, videos, and tours.
7. Advance time weekly.
8. Review sales, streams, charts, reviews, fan growth, cash flow, artist morale, and news.
9. React to opportunities, scandals, trends, contract expirations, rival labels, and market shifts.
10. Grow the label across eras, formats, regions, genres, and prestige tiers.

## Major Gameplay Features

- Campaign setup: label name, starting era, country/region, difficulty, starting capital, label identity.
- Artist discovery: auditions, demo submissions, scouts, local scenes, referrals, viral discoveries.
- Artist evaluation: talent, songwriting, charisma, reliability, ambition, ego, market appeal, genre fit.
- Artist personality: morale, burnout, loyalty, scandal risk, work ethic, band cohesion.
- Contract negotiation: advance, royalty rate, term length, album commitment, options, tour split, merch split, creative control.
- Roster management: signed artists, unsigned prospects, inactive artists, retired artists, released artists.
- Song catalog: song metadata, genre, mood, quality, hit potential, lyrical themes, era fit, ownership.
- Recording system: studio booking, producer choice, session cost, recording duration, production quality, risk of delays.
- Release planning: single, EP, album, soundtrack, live album, compilation, deluxe edition.
- Format system: vinyl, cassette, CD, digital download, streaming, region and era availability.
- Marketing system: radio, press, TV, posters, music videos, social media, influencers, playlisting, PR stunts.
- Market simulation: regions, audience segments, genre demand, cultural trends, format adoption.
- Chart system: weekly regional/global charts, genre charts, album charts, singles charts, historical peaks.
- Sales/streaming simulation: launch spike, long tail, catalog decay, radio lift, playlist lift, tour lift.
- Finance system: cash, ledger, income, expenses, advances, royalties, recording costs, marketing costs, tour income.
- News/event system: signings, releases, scandals, awards, trends, chart milestones, rival label moves.
- Reviews/critics: critic score, publications, prestige impact, fan reception.
- Awards system: nominations, wins, genre awards, industry recognition.
- Tour system: venues, routing, ticket pricing, capacity, fatigue, show quality, merch revenue.
- Rival labels: competing signings, release clashes, poaching, chart competition.
- Staff system: scouts, producers, PR managers, accountants, A&R, tour managers.
- Label progression: reputation, office upgrades, studio access, distribution deals, region unlocks.
- Era progression: shifting genres, formats, economics, media channels, chart formulas.
- Save/load: full campaign state, versioned saves, autosave, manual save slots.
- Tutorial/onboarding: guided first signing, first record, first release, first chart result.
- Accessibility: scalable UI, readable contrast, remappable inputs, reduced motion.

## Main GUI Screens

- Main Menu
- New Campaign Setup
- Load/Save Campaign
- Label Dashboard
- Artist Discovery
- Audition Screen
- Artist Detail
- Signed Roster
- Contract Negotiation
- Contract Overview
- Song Catalog
- Recording Studio
- Record Builder
- Release Planner
- Marketing Planner
- Music Video Planner
- Market/Region Map
- Charts
- Finance Dashboard
- Ledger Detail
- News Feed
- Event Detail
- Tour Planner
- Tour Results
- Reviews & Critics
- Awards
- Rival Labels
- Staff/Office
- Settings

## Dashboard GUI

The dashboard should show the player's current operating state at a glance:

- Current date/week
- Cash balance
- Monthly profit/loss
- Active releases
- Upcoming releases
- Artists needing attention
- New news/events
- Top charting release
- Label reputation
- Current market trend
- Fast-forward controls
- Save/settings access

## Artist Discovery GUI

- Featured unsigned artist
- Artist portrait
- Genre
- Age/era/context
- Talent indicators
- Commercial potential
- Risk profile
- Expected contract cost
- Scout notes
- Listen/audition button
- Pass/shortlist/negotiate actions
- Discovery filters by genre, region, cost, potential

## Audition GUI

- Artist performance presentation
- Song/performance title
- Audience reaction
- Talent stats
- Stage presence
- Vocal/instrument quality
- Songwriting quality
- Market appeal
- Negotiation sliders
- Advance
- Royalty rate
- Contract years
- Record commitment
- Sign/pass buttons
- Risk warning if deal is poor

## Artist Detail GUI

- Portrait and artist name
- Career status
- Contract status
- Genre affinities
- Talent/commercial stats
- Morale/fatigue/risk
- Active projects
- Songs available
- Records released
- Sales history
- Chart history
- Reputation
- Recommended actions
- Start recording / plan tour / renegotiate / drop artist

## Recording Studio GUI

- Selected artist
- Eligible song list
- Song quality and genre fit
- Single/EP/album selector
- Tracklist builder
- Studio tier selector
- Producer selector
- Estimated cost
- Estimated completion date
- Expected quality impact
- Confirm recording
- Cancel/back

## Release Planner GUI

- Record overview
- Release type
- Release date calendar
- Region selection
- Format selection
- Marketing budget
- Competition warning
- Era/format fit
- Predicted reach
- Expected first-week range
- Schedule release button

## Marketing GUI

- Campaign channels
- Budget allocation
- Radio push
- Press campaign
- TV/social/video/playlist options depending on era
- Region targeting
- Demographic targeting
- Expected exposure
- ROI estimate
- Campaign timeline
- Launch/adjust campaign buttons

## Charts GUI

- Weekly chart list
- Rank movement
- Artist/title
- Units/streams/chart points
- Peak position
- Weeks on chart
- Regional chart selector
- Singles/albums/genre chart tabs
- Player releases highlighted
- Historical chart view

## Finance GUI

- Cash balance
- Income/expense summary
- Monthly/quarterly/yearly toggle
- Ledger entries
- Category breakdown
- Artist profitability
- Release profitability
- Marketing ROI
- Tour profitability
- Royalty obligations
- Bankruptcy warnings

## News GUI

- News ticker
- Full news feed
- Filter by artist, finance, charts, market, rival labels, scandals
- News card detail
- Gameplay impact display
- Related action button when applicable

## Market/Region GUI

- Interactive region map
- Demand by genre
- Audience size
- Format adoption
- Radio/streaming/press strength
- Player label presence
- Rival label presence
- Top local artists
- Recommended campaign actions

## Tour GUI

- Artist selection
- Route map/calendar
- Venue list
- Capacity
- Ticket price
- Estimated demand
- Cost/revenue projection
- Fatigue risk
- Show-by-show results
- Tour summary

## Professional Polish Requirements

- Every screen must use real game data.
- No mock GUI, placeholder values, or stub buttons.
- Every action should validate and give clear feedback.
- All GUI should follow `docs/design.md`: dark premium studio style, black vinyl motifs, warm gold accents.
- Before implementing any GUI, generate a production-quality reference image and match the widget against it.
- Each major screen should have empty states, loading states, error states, and disabled states.
- All important decisions should show consequences before confirmation.
- The player should always know what changed after advancing time.

