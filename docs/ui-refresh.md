# Studio UI refresh

A native JUCE presentation layer for the existing GuitarRack workflow.

- Dark studio surfaces, mint primary actions and consistent menus, text fields,
  dialogs, disabled states and keyboard focus outlines.
- Colour-coded pedal enclosures: warm amps, coral distortion/fuzz, green
  overdrive/boost, blue delay, violet space/pitch, cyan modulation and yellow EQ.
  Engaged/bypassed text remains visible alongside enclosure/LED colour.
- Larger rotary controls with value arcs, shaded knobs, editable numeric values
  and more space between parameters. Existing drag, reset and unit mappings stay.
- Two-line preset cards show the existing artist/gain metadata and pedal count.
  Filter captions, result count and empty-search guidance clarify browsing.
- Start/Stop/Mute remain visible above the audio settings viewport. On short
  displays the settings scroll independently of the pedalboard. No new setting
  or persistence format is introduced.

No new DSP, filters, preset recipes, automatic audition, animations, action
handlers, menu commands, external assets or dependencies. Native file choosers
continue to use the operating system's appearance. The application owns the
shared LookAndFeel and releases it after closing its windows.

## Desktop acceptance (pending)

Build and launch using the existing README commands. This environment compiled
UI objects but could not link or display the app; see the exact limits in
[testing.md](testing.md).

1. Inspect 1280×720, 1366×768, a wide display and 150% scaling. Resize to 700×450.
   Expand Audio Setup; check its scroll area, fixed transport and board scrolling.
2. Add an amp, drive, Echo and Cabinet; inspect compact and all-controls layouts,
   long pedal/voicing names, IR controls, bypass states and numeric entry. Change
   values, double-click reset, then move pedals Earlier/Later.
3. Open Choose Preset at normal and minimum size. Check long names, selected rows,
   search with no matches, filter captions, Favorites and the recipe pane. Tab
   through fields/buttons and verify visible focus, Enter, Escape and double-click.
4. Inspect Save As and unsaved-change alerts, disabled buttons, Delete confirmation,
   popup menus, file choosers and closing with the browser open. Confirm existing
   cancellation, loading, startup silence and preset state behavior.

Visual acceptance does not replace the project's outstanding guitar/hardware,
latency, soak, Windows or VST3 acceptance.
