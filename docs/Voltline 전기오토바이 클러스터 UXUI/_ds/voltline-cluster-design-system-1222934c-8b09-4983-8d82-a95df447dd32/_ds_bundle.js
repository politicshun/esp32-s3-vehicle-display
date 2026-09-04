/* @ds-bundle: {"format":4,"namespace":"VoltlineClusterDesignSystem_122293","components":[{"name":"ArcGauge","sourcePath":"components/cluster/ArcGauge.jsx"},{"name":"BatteryGauge","sourcePath":"components/cluster/BatteryGauge.jsx"},{"name":"PowerFlowBar","sourcePath":"components/cluster/PowerFlowBar.jsx"},{"name":"RideModeSelector","sourcePath":"components/cluster/RideModeSelector.jsx"},{"name":"SpeedReadout","sourcePath":"components/cluster/SpeedReadout.jsx"},{"name":"Telltale","sourcePath":"components/cluster/Telltale.jsx"},{"name":"TelltaleRail","sourcePath":"components/cluster/TelltaleRail.jsx"},{"name":"TripStat","sourcePath":"components/cluster/TripStat.jsx"},{"name":"Badge","sourcePath":"components/core/Badge.jsx"},{"name":"Button","sourcePath":"components/core/Button.jsx"},{"name":"Icon","sourcePath":"components/core/Icon.jsx"},{"name":"IconButton","sourcePath":"components/core/IconButton.jsx"},{"name":"Panel","sourcePath":"components/core/Panel.jsx"},{"name":"AlertBanner","sourcePath":"components/feedback/AlertBanner.jsx"},{"name":"Dialog","sourcePath":"components/feedback/Dialog.jsx"},{"name":"Toast","sourcePath":"components/feedback/Toast.jsx"},{"name":"SegmentedControl","sourcePath":"components/forms/SegmentedControl.jsx"},{"name":"Slider","sourcePath":"components/forms/Slider.jsx"},{"name":"Switch","sourcePath":"components/forms/Switch.jsx"},{"name":"TextField","sourcePath":"components/forms/TextField.jsx"}],"sourceHashes":{"components/cluster/ArcGauge.jsx":"d385effeb4cc","components/cluster/BatteryGauge.jsx":"6a48c8cddd91","components/cluster/PowerFlowBar.jsx":"6bce71ef1947","components/cluster/RideModeSelector.jsx":"cc412ec65d12","components/cluster/SpeedReadout.jsx":"be0cd1e12d7a","components/cluster/Telltale.jsx":"3b80f0a2fd70","components/cluster/TelltaleRail.jsx":"1072f0ad2583","components/cluster/TripStat.jsx":"9b5ff752ecfa","components/core/Badge.jsx":"3169b4781d59","components/core/Button.jsx":"8dd3c2563dc6","components/core/Icon.jsx":"ed85eea1524c","components/core/IconButton.jsx":"ea59e4620f15","components/core/Panel.jsx":"59849153afcd","components/feedback/AlertBanner.jsx":"035a0080fed7","components/feedback/Dialog.jsx":"859d4bde6602","components/feedback/Toast.jsx":"d4520f6c7e7f","components/forms/SegmentedControl.jsx":"b0e419ac4910","components/forms/Slider.jsx":"3613672f9b27","components/forms/Switch.jsx":"6b64afce23bf","components/forms/TextField.jsx":"8e05528f7b57","ui_kits/cluster/ClusterApp.jsx":"f3036cdab5d3","ui_kits/cluster/DiagnosticsScreen.jsx":"e2e098bc3db3","ui_kits/cluster/NavScreen.jsx":"dc23a2012d5f","ui_kits/cluster/RideScreen.jsx":"644c2f159646","ui_kits/cluster/SettingsScreen.jsx":"7979c22b8c78","ui_kits/cluster/TripScreen.jsx":"15e074f7c862","ui_kits/companion-app/BikeScreen.jsx":"49d142614ea3","ui_kits/companion-app/CompanionApp.jsx":"9db2a336ceb1","ui_kits/companion-app/RideDetailScreen.jsx":"9e59a7410f36","ui_kits/companion-app/RidesScreen.jsx":"252ba37c5e0a","ui_kits/companion-app/SetupScreen.jsx":"2504d78f4f14"},"inlinedExternals":[],"unexposedExports":[]} */

(() => {

const __ds_ns = (window.VoltlineClusterDesignSystem_122293 = window.VoltlineClusterDesignSystem_122293 || {});

const __ds_scope = {};

(__ds_ns.__errors = __ds_ns.__errors || []);

// components/cluster/ArcGauge.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Sweep arc used behind the speed readout and for power/charge dials. */
function ArcGauge({
  value = 0,
  max = 100,
  size = 260,
  thickness = 12,
  tone = 'accent',
  label,
  children,
  style,
  ...rest
}) {
  const color = tone === 'critical' ? 'var(--signal-critical)' : tone === 'caution' ? 'var(--signal-caution)' : tone === 'go' ? 'var(--signal-go)' : 'var(--current-500)';
  const pct = Math.max(0, Math.min(1, value / max));
  const SWEEP = 260,
    START = 140;
  const deg = SWEEP * pct;
  return /*#__PURE__*/React.createElement("div", _extends({}, rest, {
    style: {
      position: 'relative',
      width: size,
      height: size,
      display: 'grid',
      placeItems: 'center',
      ...style
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      inset: 0,
      borderRadius: '50%',
      background: `conic-gradient(from ${START}deg, ${color} 0deg ${deg}deg, var(--track-empty) ${deg}deg ${SWEEP}deg, transparent ${SWEEP}deg 360deg)`,
      WebkitMask: `radial-gradient(circle, transparent calc(50% - ${thickness}px), #000 calc(50% - ${thickness}px))`,
      mask: `radial-gradient(circle, transparent calc(50% - ${thickness}px), #000 calc(50% - ${thickness}px))`,
      transition: 'background var(--dur-base) var(--ease-readout)'
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      placeItems: 'center',
      textAlign: 'center'
    }
  }, children, label && /*#__PURE__*/React.createElement("span", {
    style: {
      marginTop: 'var(--space-2)',
      fontSize: 'var(--type-caption)',
      letterSpacing: 'var(--tracking-micro)',
      textTransform: 'uppercase',
      color: 'var(--text-tertiary)'
    }
  }, label)));
}
Object.assign(__ds_scope, { ArcGauge });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/cluster/ArcGauge.jsx", error: String((e && e.message) || e) }); }

// components/cluster/PowerFlowBar.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Bidirectional motor power meter: regen left of zero, draw right of zero. */
function PowerFlowBar({
  watts = 0,
  maxDraw = 750,
  maxRegen = 300,
  style,
  ...rest
}) {
  const regen = watts < 0;
  const pct = regen ? Math.min(1, -watts / maxRegen) : Math.min(1, watts / maxDraw);
  const zero = maxRegen / (maxRegen + maxDraw) * 100;
  const w = pct * (regen ? zero : 100 - zero);
  return /*#__PURE__*/React.createElement("div", _extends({}, rest, {
    style: {
      ...style
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      justifyContent: 'space-between',
      alignItems: 'baseline',
      marginBottom: 'var(--space-2)'
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--type-caption)',
      letterSpacing: 'var(--tracking-label)',
      textTransform: 'uppercase',
      color: 'var(--text-tertiary)'
    }
  }, regen ? 'Regen' : 'Power'), /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: 'var(--font-readout)',
      fontSize: 'var(--type-body-lg)',
      fontWeight: 'var(--weight-semibold)',
      color: regen ? 'var(--energy-regen)' : 'var(--text-primary)',
      fontVariantNumeric: 'tabular-nums'
    }
  }, Math.abs(Math.round(watts)), /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--type-caption)',
      color: 'var(--text-tertiary)'
    }
  }, " W"))), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'relative',
      height: 10,
      background: 'var(--track-empty)',
      borderRadius: 'var(--radius-pill)',
      overflow: 'hidden'
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      top: 0,
      bottom: 0,
      left: `${zero}%`,
      width: 1,
      background: 'var(--line-strong)',
      zIndex: 1
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      top: 0,
      bottom: 0,
      left: regen ? `${zero - w}%` : `${zero}%`,
      width: `${w}%`,
      background: regen ? 'var(--energy-regen)' : 'var(--current-500)',
      transition: 'all var(--dur-fast) var(--ease-standard)'
    }
  })));
}
Object.assign(__ds_scope, { PowerFlowBar });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/cluster/PowerFlowBar.jsx", error: String((e && e.message) || e) }); }

// components/cluster/SpeedReadout.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** The primary readout. One per screen, always the largest element on the display. */
function SpeedReadout({
  value = 0,
  unit = 'km/h',
  size = 'xl',
  limit,
  style,
  ...rest
}) {
  const fs = size === 'xl' ? 'var(--type-speed-xl)' : size === 'lg' ? 'var(--type-speed)' : 'var(--type-readout-1)';
  const over = limit != null && value > limit;
  return /*#__PURE__*/React.createElement("div", _extends({}, rest, {
    style: {
      display: 'flex',
      flexDirection: 'column',
      alignItems: 'center',
      ...style
    }
  }), /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: 'var(--font-readout)',
      fontSize: fs,
      fontWeight: 'var(--weight-semibold)',
      lineHeight: 'var(--leading-readout)',
      letterSpacing: 'var(--tracking-readout)',
      fontVariantNumeric: 'tabular-nums',
      color: over ? 'var(--signal-caution)' : 'var(--text-primary)',
      transition: 'color var(--dur-base) var(--ease-standard)'
    }
  }, Math.round(value)), /*#__PURE__*/React.createElement("span", {
    style: {
      marginTop: 'var(--space-2)',
      fontFamily: 'var(--font-core)',
      fontSize: 'var(--type-label)',
      fontWeight: 'var(--weight-medium)',
      letterSpacing: 'var(--tracking-micro)',
      textTransform: 'uppercase',
      color: 'var(--text-tertiary)'
    }
  }, unit));
}
Object.assign(__ds_scope, { SpeedReadout });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/cluster/SpeedReadout.jsx", error: String((e && e.message) || e) }); }

// components/core/Icon.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
const CDN = 'https://unpkg.com/lucide-static@0.400.0/icons/';

/** Monochrome glyph. Renders a Lucide outline icon as a CSS mask so it always
 *  inherits currentColor — never an <img>, which would ignore signal colors. */
function Icon({
  name,
  size = 20,
  strokeAccent,
  style,
  ...rest
}) {
  const url = `url("${CDN}${name}.svg")`;
  return /*#__PURE__*/React.createElement("span", _extends({
    "aria-hidden": "true"
  }, rest, {
    style: {
      display: 'inline-block',
      width: size,
      height: size,
      flex: '0 0 auto',
      background: strokeAccent || 'currentColor',
      WebkitMaskImage: url,
      maskImage: url,
      WebkitMaskRepeat: 'no-repeat',
      maskRepeat: 'no-repeat',
      WebkitMaskPosition: 'center',
      maskPosition: 'center',
      WebkitMaskSize: 'contain',
      maskSize: 'contain',
      ...style
    }
  }));
}
Object.assign(__ds_scope, { Icon });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/core/Icon.jsx", error: String((e && e.message) || e) }); }

// components/cluster/BatteryGauge.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
const rampColor = p => p > 55 ? 'var(--energy-full)' : p > 30 ? 'var(--energy-mid)' : p > 12 ? 'var(--energy-low)' : 'var(--energy-empty)';

/** State of charge as a segmented cell bar plus percentage and range estimate. */
function BatteryGauge({
  percent = 0,
  rangeKm,
  unit = 'km',
  charging,
  segments = 10,
  orientation = 'horizontal',
  style,
  ...rest
}) {
  const color = charging ? 'var(--energy-regen)' : rampColor(percent);
  const filled = Math.round(percent / 100 * segments);
  const vertical = orientation === 'vertical';
  return /*#__PURE__*/React.createElement("div", _extends({}, rest, {
    style: {
      display: 'flex',
      flexDirection: 'column',
      gap: 'var(--space-3)',
      ...style
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      alignItems: 'baseline',
      gap: 'var(--space-3)'
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      color,
      display: 'flex',
      alignSelf: 'center'
    }
  }, /*#__PURE__*/React.createElement(__ds_scope.Icon, {
    name: charging ? 'battery-charging' : 'battery',
    size: 22
  })), /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: 'var(--font-readout)',
      fontSize: 'var(--type-readout-3)',
      fontWeight: 'var(--weight-semibold)',
      lineHeight: 1,
      color: 'var(--text-primary)',
      fontVariantNumeric: 'tabular-nums'
    }
  }, Math.round(percent)), /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--type-label)',
      color: 'var(--text-tertiary)'
    }
  }, "%"), rangeKm != null && /*#__PURE__*/React.createElement("span", {
    style: {
      marginLeft: 'auto',
      fontFamily: 'var(--font-readout)',
      fontSize: 'var(--type-body-lg)',
      color: 'var(--text-secondary)',
      fontVariantNumeric: 'tabular-nums'
    }
  }, rangeKm, /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--type-caption)',
      color: 'var(--text-tertiary)'
    }
  }, " ", unit, " left"))), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      flexDirection: vertical ? 'column-reverse' : 'row',
      gap: 3,
      height: vertical ? 120 : 14
    }
  }, Array.from({
    length: segments
  }).map((_, i) => /*#__PURE__*/React.createElement("span", {
    key: i,
    style: {
      flex: 1,
      background: i < filled ? color : 'var(--track-empty)',
      borderRadius: 2,
      transition: 'background var(--dur-base) var(--ease-standard)'
    }
  }))));
}
Object.assign(__ds_scope, { BatteryGauge });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/cluster/BatteryGauge.jsx", error: String((e && e.message) || e) }); }

// components/cluster/RideModeSelector.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Assist / power mode picker. Fixed set per vehicle; the current mode is always visible. */
function RideModeSelector({
  modes = [],
  value,
  onChange,
  orientation = 'horizontal',
  style,
  ...rest
}) {
  const vertical = orientation === 'vertical';
  return /*#__PURE__*/React.createElement("div", _extends({
    role: "radiogroup"
  }, rest, {
    style: {
      display: 'flex',
      flexDirection: vertical ? 'column' : 'row',
      gap: 'var(--space-2)',
      ...style
    }
  }), modes.map(m => {
    const sel = m.value === value;
    return /*#__PURE__*/React.createElement("button", {
      key: m.value,
      role: "radio",
      "aria-checked": sel,
      onClick: () => onChange && onChange(m.value),
      style: {
        flex: 1,
        display: 'flex',
        flexDirection: vertical ? 'row' : 'column',
        alignItems: 'center',
        justifyContent: vertical ? 'flex-start' : 'center',
        gap: 'var(--space-2)',
        minHeight: 'var(--hit-gloved)',
        padding: 'var(--space-3) var(--space-4)',
        background: sel ? 'rgba(0,229,208,.12)' : 'var(--surface-raised)',
        border: `var(--border-emphasis) solid ${sel ? 'var(--current-500)' : 'transparent'}`,
        borderRadius: 'var(--radius-tile)',
        cursor: 'pointer',
        color: sel ? 'var(--current-400)' : 'var(--text-tertiary)',
        transition: 'all var(--dur-fast) var(--ease-standard)'
      }
    }, m.icon && /*#__PURE__*/React.createElement(__ds_scope.Icon, {
      name: m.icon,
      size: 20
    }), /*#__PURE__*/React.createElement("span", {
      style: {
        fontFamily: 'var(--font-core)',
        fontSize: 'var(--type-label)',
        fontWeight: 'var(--weight-semibold)',
        letterSpacing: 'var(--tracking-label)',
        textTransform: 'uppercase'
      }
    }, m.label));
  }));
}
Object.assign(__ds_scope, { RideModeSelector });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/cluster/RideModeSelector.jsx", error: String((e && e.message) || e) }); }

// components/cluster/Telltale.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
const TONES = {
  turn: 'var(--signal-go)',
  beam: 'var(--signal-beam)',
  go: 'var(--signal-go)',
  caution: 'var(--signal-caution)',
  critical: 'var(--signal-critical)',
  info: 'var(--current-500)'
};

/** Single indicator lamp. Off = dimmed to 18%, never hidden — riders learn fixed positions. */
function Telltale({
  icon,
  tone = 'info',
  active,
  blink,
  size = 32,
  label,
  style,
  ...rest
}) {
  const color = TONES[tone] || TONES.info;
  return /*#__PURE__*/React.createElement("span", _extends({
    title: label,
    "aria-label": label
  }, rest, {
    style: {
      display: 'inline-flex',
      alignItems: 'center',
      justifyContent: 'center',
      width: size + 12,
      height: size + 12,
      color,
      opacity: active ? 1 : 0.18,
      filter: active ? `drop-shadow(0 0 10px ${color})` : 'none',
      animation: active && blink ? `voltline-blink var(--pulse-turn) steps(1,end) infinite` : 'none',
      transition: 'opacity var(--dur-instant) var(--ease-standard)',
      ...style
    }
  }), /*#__PURE__*/React.createElement(__ds_scope.Icon, {
    name: icon,
    size: size
  }));
}
Object.assign(__ds_scope, { Telltale });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/cluster/Telltale.jsx", error: String((e && e.message) || e) }); }

// components/cluster/TelltaleRail.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** The fixed lamp row across the top of every cluster screen. Order never changes. */
function TelltaleRail({
  items = [],
  align = 'center',
  size = 30,
  style,
  ...rest
}) {
  return /*#__PURE__*/React.createElement("div", _extends({}, rest, {
    style: {
      display: 'flex',
      alignItems: 'center',
      justifyContent: align === 'center' ? 'center' : 'flex-start',
      gap: 'var(--space-4)',
      height: 'var(--bar-status)',
      ...style
    }
  }), items.map((t, i) => /*#__PURE__*/React.createElement(__ds_scope.Telltale, _extends({
    key: t.label || i,
    size: size
  }, t))));
}
Object.assign(__ds_scope, { TelltaleRail });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/cluster/TelltaleRail.jsx", error: String((e && e.message) || e) }); }

// components/cluster/TripStat.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** One labelled metric. The building block of trip, history and diagnostics panels. */
function TripStat({
  label,
  value,
  unit,
  icon,
  size = 'md',
  tone,
  style,
  ...rest
}) {
  const fs = size === 'lg' ? 'var(--type-readout-2)' : size === 'sm' ? 'var(--type-body-lg)' : 'var(--type-readout-3)';
  return /*#__PURE__*/React.createElement("div", _extends({}, rest, {
    style: {
      display: 'flex',
      flexDirection: 'column',
      gap: 'var(--space-1)',
      ...style
    }
  }), /*#__PURE__*/React.createElement("span", {
    style: {
      display: 'flex',
      alignItems: 'center',
      gap: 'var(--space-2)',
      fontSize: 'var(--type-caption)',
      letterSpacing: 'var(--tracking-label)',
      textTransform: 'uppercase',
      color: 'var(--text-tertiary)'
    }
  }, icon && /*#__PURE__*/React.createElement(__ds_scope.Icon, {
    name: icon,
    size: 13
  }), label), /*#__PURE__*/React.createElement("span", {
    style: {
      display: 'flex',
      alignItems: 'baseline',
      gap: 'var(--space-2)'
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: 'var(--font-readout)',
      fontSize: fs,
      fontWeight: 'var(--weight-semibold)',
      lineHeight: 1,
      letterSpacing: 'var(--tracking-readout)',
      color: tone === 'accent' ? 'var(--text-accent)' : 'var(--text-primary)',
      fontVariantNumeric: 'tabular-nums'
    }
  }, value), unit && /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--type-label)',
      color: 'var(--text-tertiary)'
    }
  }, unit)));
}
Object.assign(__ds_scope, { TripStat });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/cluster/TripStat.jsx", error: String((e && e.message) || e) }); }

// components/core/Badge.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
const TONES = {
  neutral: ['var(--ink-700)', 'var(--text-secondary)'],
  accent: ['rgba(0,229,208,.14)', 'var(--current-400)'],
  go: ['rgba(62,210,107,.14)', 'var(--signal-go)'],
  caution: ['rgba(255,176,32,.14)', 'var(--signal-caution)'],
  critical: ['rgba(255,68,56,.16)', 'var(--signal-critical)']
};

/** Compact status label. Never interactive. */
function Badge({
  tone = 'neutral',
  icon,
  children,
  style,
  ...rest
}) {
  const [bg, fg] = TONES[tone] || TONES.neutral;
  return /*#__PURE__*/React.createElement("span", _extends({}, rest, {
    style: {
      display: 'inline-flex',
      alignItems: 'center',
      gap: 'var(--space-2)',
      height: 24,
      padding: '0 10px',
      background: bg,
      color: fg,
      borderRadius: 'var(--radius-pill)',
      fontFamily: 'var(--font-core)',
      fontSize: 'var(--type-caption)',
      fontWeight: 'var(--weight-semibold)',
      letterSpacing: 'var(--tracking-label)',
      textTransform: 'uppercase',
      ...style
    }
  }), icon && /*#__PURE__*/React.createElement(__ds_scope.Icon, {
    name: icon,
    size: 13
  }), children);
}
Object.assign(__ds_scope, { Badge });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/core/Badge.jsx", error: String((e && e.message) || e) }); }

// components/core/Button.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
const TONES = {
  primary: {
    bg: 'var(--current-500)',
    fg: 'var(--ink-950)',
    bd: 'transparent'
  },
  secondary: {
    bg: 'var(--surface-raised)',
    fg: 'var(--text-primary)',
    bd: 'var(--line-default)'
  },
  ghost: {
    bg: 'transparent',
    fg: 'var(--text-secondary)',
    bd: 'transparent'
  },
  critical: {
    bg: 'var(--signal-critical)',
    fg: 'var(--ink-000)',
    bd: 'transparent'
  }
};
const SIZES = {
  sm: {
    h: 36,
    px: 14,
    fs: 'var(--type-label)',
    icon: 16
  },
  md: {
    h: 44,
    px: 18,
    fs: 'var(--type-body)',
    icon: 20
  },
  lg: {
    h: 56,
    px: 24,
    fs: 'var(--type-body-lg)',
    icon: 22
  }
};

/** Primary action control. Gloved use: prefer size="lg" on cluster surfaces. */
function Button({
  tone = 'primary',
  size = 'md',
  icon,
  iconRight,
  block,
  disabled,
  children,
  style,
  ...rest
}) {
  const t = TONES[tone] || TONES.primary;
  const s = SIZES[size] || SIZES.md;
  return /*#__PURE__*/React.createElement("button", _extends({
    disabled: disabled
  }, rest, {
    style: {
      display: block ? 'flex' : 'inline-flex',
      width: block ? '100%' : undefined,
      alignItems: 'center',
      justifyContent: 'center',
      gap: 'var(--space-3)',
      height: s.h,
      padding: `0 ${s.px}px`,
      minWidth: s.h,
      font: 'inherit',
      fontFamily: 'var(--font-core)',
      fontSize: s.fs,
      fontWeight: 'var(--weight-semibold)',
      letterSpacing: '0.01em',
      color: disabled ? 'var(--text-disabled)' : t.fg,
      background: disabled ? 'var(--ink-800)' : t.bg,
      border: `var(--border-default) solid ${disabled ? 'var(--line-hairline)' : t.bd}`,
      borderRadius: 'var(--radius-control)',
      cursor: disabled ? 'not-allowed' : 'pointer',
      transition: 'filter var(--dur-fast) var(--ease-standard), transform var(--dur-instant) var(--ease-standard)',
      ...style
    },
    onPointerDown: e => {
      if (!disabled) e.currentTarget.style.transform = 'scale(.97)';
    },
    onPointerUp: e => {
      e.currentTarget.style.transform = 'none';
    },
    onPointerLeave: e => {
      e.currentTarget.style.transform = 'none';
    }
  }), icon && /*#__PURE__*/React.createElement(__ds_scope.Icon, {
    name: icon,
    size: s.icon
  }), children, iconRight && /*#__PURE__*/React.createElement(__ds_scope.Icon, {
    name: iconRight,
    size: s.icon
  }));
}
Object.assign(__ds_scope, { Button });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/core/Button.jsx", error: String((e && e.message) || e) }); }

// components/core/IconButton.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
const SIZES = {
  sm: 36,
  md: 44,
  lg: 56
};

/** Square glyph-only control. Always pass a label — clusters are audited for a11y. */
function IconButton({
  icon,
  label,
  size = 'md',
  tone = 'secondary',
  active,
  disabled,
  style,
  ...rest
}) {
  const d = SIZES[size] || SIZES.md;
  const bg = active ? 'var(--current-500)' : tone === 'ghost' ? 'transparent' : 'var(--surface-raised)';
  return /*#__PURE__*/React.createElement("button", _extends({
    "aria-label": label,
    title: label,
    disabled: disabled
  }, rest, {
    style: {
      display: 'inline-flex',
      alignItems: 'center',
      justifyContent: 'center',
      width: d,
      height: d,
      padding: 0,
      color: active ? 'var(--ink-950)' : disabled ? 'var(--text-disabled)' : 'var(--text-secondary)',
      background: disabled ? 'var(--ink-800)' : bg,
      border: `var(--border-default) solid ${tone === 'ghost' || active ? 'transparent' : 'var(--line-default)'}`,
      borderRadius: 'var(--radius-control)',
      cursor: disabled ? 'not-allowed' : 'pointer',
      transition: 'background var(--dur-fast) var(--ease-standard), color var(--dur-fast) var(--ease-standard)',
      ...style
    }
  }), /*#__PURE__*/React.createElement(__ds_scope.Icon, {
    name: icon,
    size: Math.round(d * 0.46)
  }));
}
Object.assign(__ds_scope, { IconButton });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/core/IconButton.jsx", error: String((e && e.message) || e) }); }

// components/core/Panel.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Instrument panel surface. The only container in the system — cards, tiles and
 *  sheets are all Panels at different densities. */
function Panel({
  title,
  meta,
  tone = 'default',
  density = 'comfortable',
  children,
  style,
  ...rest
}) {
  const pad = density === 'compact' ? 'var(--space-4)' : 'var(--space-6)';
  const accent = tone === 'accent';
  return /*#__PURE__*/React.createElement("section", _extends({}, rest, {
    style: {
      background: 'var(--surface-panel)',
      border: `var(--border-default) solid ${accent ? 'var(--line-accent)' : 'var(--line-hairline)'}`,
      borderRadius: 'var(--radius-panel)',
      padding: pad,
      boxShadow: accent ? 'var(--glow-accent)' : 'var(--shadow-tile)',
      ...style
    }
  }), (title || meta) && /*#__PURE__*/React.createElement("header", {
    style: {
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'space-between',
      gap: 'var(--space-4)',
      marginBottom: 'var(--space-4)'
    }
  }, title && /*#__PURE__*/React.createElement("h3", {
    style: {
      margin: 0,
      fontFamily: 'var(--font-core)',
      fontSize: 'var(--type-label)',
      fontWeight: 'var(--weight-semibold)',
      letterSpacing: 'var(--tracking-label)',
      textTransform: 'uppercase',
      color: 'var(--text-secondary)'
    }
  }, title), meta && /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--type-caption)',
      color: 'var(--text-tertiary)',
      fontFamily: 'var(--font-mono)'
    }
  }, meta)), children);
}
Object.assign(__ds_scope, { Panel });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/core/Panel.jsx", error: String((e && e.message) || e) }); }

// components/feedback/AlertBanner.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
const TONES = {
  info: ['var(--current-500)', 'rgba(0,229,208,.10)'],
  caution: ['var(--signal-caution)', 'rgba(255,176,32,.12)'],
  critical: ['var(--signal-critical)', 'rgba(255,68,56,.14)']
};
const ICONS = {
  info: 'info',
  caution: 'triangle-alert',
  critical: 'octagon-alert'
};

/** Persistent in-context warning. On the cluster it docks below the telltale rail. */
function AlertBanner({
  tone = 'caution',
  title,
  detail,
  code,
  action,
  style,
  ...rest
}) {
  const [fg, bg] = TONES[tone] || TONES.caution;
  return /*#__PURE__*/React.createElement("div", _extends({
    role: "alert"
  }, rest, {
    style: {
      display: 'flex',
      alignItems: 'center',
      gap: 'var(--space-4)',
      padding: 'var(--space-4) var(--space-5)',
      background: bg,
      border: `var(--border-default) solid ${fg}`,
      borderRadius: 'var(--radius-tile)',
      ...style
    }
  }), /*#__PURE__*/React.createElement("span", {
    style: {
      color: fg,
      display: 'flex'
    }
  }, /*#__PURE__*/React.createElement(__ds_scope.Icon, {
    name: ICONS[tone],
    size: 24
  })), /*#__PURE__*/React.createElement("div", {
    style: {
      flex: 1,
      minWidth: 0
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      fontSize: 'var(--type-body)',
      fontWeight: 'var(--weight-semibold)',
      color: 'var(--text-primary)'
    }
  }, title), detail && /*#__PURE__*/React.createElement("div", {
    style: {
      fontSize: 'var(--type-caption)',
      color: 'var(--text-secondary)'
    }
  }, detail)), code && /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: 'var(--font-mono)',
      fontSize: 'var(--type-caption)',
      color: fg
    }
  }, code), action);
}
Object.assign(__ds_scope, { AlertBanner });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/feedback/AlertBanner.jsx", error: String((e && e.message) || e) }); }

// components/feedback/Dialog.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Blocking confirmation. Only for destructive or safety-relevant choices. */
function Dialog({
  open = true,
  title,
  children,
  actions,
  width = 420,
  style,
  ...rest
}) {
  if (!open) return null;
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      inset: 0,
      display: 'grid',
      placeItems: 'center',
      background: 'var(--scrim)',
      backdropFilter: 'blur(var(--blur-scrim))',
      zIndex: 40
    }
  }, /*#__PURE__*/React.createElement("div", _extends({
    role: "dialog",
    "aria-modal": "true"
  }, rest, {
    style: {
      width,
      maxWidth: '90%',
      padding: 'var(--space-7)',
      background: 'var(--surface-panel)',
      border: 'var(--border-default) solid var(--line-default)',
      borderRadius: 'var(--radius-panel)',
      boxShadow: 'var(--shadow-modal)',
      ...style
    }
  }), /*#__PURE__*/React.createElement("h2", {
    style: {
      margin: '0 0 var(--space-3)',
      fontFamily: 'var(--font-core)',
      fontSize: 'var(--type-title)',
      fontWeight: 'var(--weight-semibold)',
      lineHeight: 'var(--leading-tight)'
    }
  }, title), /*#__PURE__*/React.createElement("div", {
    style: {
      fontSize: 'var(--type-body)',
      color: 'var(--text-secondary)'
    }
  }, children), actions && /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      gap: 'var(--space-3)',
      justifyContent: 'flex-end',
      marginTop: 'var(--space-7)'
    }
  }, actions)));
}
Object.assign(__ds_scope, { Dialog });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/feedback/Dialog.jsx", error: String((e && e.message) || e) }); }

// components/feedback/Toast.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Transient confirmation. Auto-dismisses; never carries an action a rider must take. */
function Toast({
  icon = 'check',
  tone = 'neutral',
  children,
  style,
  ...rest
}) {
  const fg = tone === 'go' ? 'var(--signal-go)' : tone === 'caution' ? 'var(--signal-caution)' : 'var(--current-400)';
  return /*#__PURE__*/React.createElement("div", _extends({
    role: "status"
  }, rest, {
    style: {
      display: 'inline-flex',
      alignItems: 'center',
      gap: 'var(--space-3)',
      padding: 'var(--space-4) var(--space-5)',
      background: 'var(--ink-800)',
      border: 'var(--border-default) solid var(--line-default)',
      borderRadius: 'var(--radius-pill)',
      boxShadow: 'var(--shadow-panel)',
      fontSize: 'var(--type-body)',
      color: 'var(--text-primary)',
      ...style
    }
  }), /*#__PURE__*/React.createElement("span", {
    style: {
      color: fg,
      display: 'flex'
    }
  }, /*#__PURE__*/React.createElement(__ds_scope.Icon, {
    name: icon,
    size: 18
  })), children);
}
Object.assign(__ds_scope, { Toast });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/feedback/Toast.jsx", error: String((e && e.message) || e) }); }

// components/forms/SegmentedControl.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** 2-4 mutually exclusive options in one track. The system's only tab pattern. */
function SegmentedControl({
  options = [],
  value,
  onChange,
  size = 'md',
  block,
  style,
  ...rest
}) {
  const h = size === 'lg' ? 56 : size === 'sm' ? 36 : 44;
  return /*#__PURE__*/React.createElement("div", _extends({
    role: "tablist"
  }, rest, {
    style: {
      display: block ? 'flex' : 'inline-flex',
      width: block ? '100%' : undefined,
      padding: 3,
      gap: 3,
      background: 'var(--surface-sunken)',
      border: 'var(--border-default) solid var(--line-hairline)',
      borderRadius: 'var(--radius-control)',
      ...style
    }
  }), options.map(o => {
    const sel = o.value === value;
    return /*#__PURE__*/React.createElement("button", {
      key: o.value,
      role: "tab",
      "aria-selected": sel,
      onClick: () => onChange && onChange(o.value),
      style: {
        flex: block ? 1 : '0 0 auto',
        display: 'inline-flex',
        alignItems: 'center',
        justifyContent: 'center',
        gap: 'var(--space-2)',
        height: h - 6,
        padding: '0 16px',
        background: sel ? 'var(--surface-raised)' : 'transparent',
        color: sel ? 'var(--text-primary)' : 'var(--text-tertiary)',
        border: `var(--border-default) solid ${sel ? 'var(--line-default)' : 'transparent'}`,
        borderRadius: 'calc(var(--radius-control) - 3px)',
        cursor: 'pointer',
        fontFamily: 'var(--font-core)',
        fontSize: size === 'sm' ? 'var(--type-caption)' : 'var(--type-label)',
        fontWeight: 'var(--weight-semibold)',
        letterSpacing: 'var(--tracking-label)',
        textTransform: 'uppercase',
        transition: 'background var(--dur-fast) var(--ease-standard), color var(--dur-fast) var(--ease-standard)'
      }
    }, o.icon && /*#__PURE__*/React.createElement(__ds_scope.Icon, {
      name: o.icon,
      size: 16
    }), o.label);
  }));
}
Object.assign(__ds_scope, { SegmentedControl });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/SegmentedControl.jsx", error: String((e && e.message) || e) }); }

// components/forms/Slider.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Continuous setting (assist level, brightness, regen strength). */
function Slider({
  value = 0,
  min = 0,
  max = 100,
  step = 1,
  onChange,
  label,
  unit,
  disabled,
  style,
  ...rest
}) {
  const pct = Math.max(0, Math.min(100, (value - min) / (max - min) * 100));
  return /*#__PURE__*/React.createElement("div", _extends({}, rest, {
    style: {
      opacity: disabled ? .5 : 1,
      ...style
    }
  }), (label || unit) && /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      justifyContent: 'space-between',
      alignItems: 'baseline',
      marginBottom: 'var(--space-3)'
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--type-label)',
      letterSpacing: 'var(--tracking-label)',
      textTransform: 'uppercase',
      color: 'var(--text-secondary)'
    }
  }, label), /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: 'var(--font-readout)',
      fontSize: 'var(--type-body-lg)',
      fontWeight: 'var(--weight-semibold)',
      color: 'var(--text-primary)',
      fontVariantNumeric: 'tabular-nums'
    }
  }, value, unit ? ` ${unit}` : '')), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'relative',
      height: 28,
      display: 'flex',
      alignItems: 'center'
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      inset: '11px 0',
      background: 'var(--track-empty)',
      borderRadius: 'var(--radius-pill)'
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: 0,
      top: 11,
      bottom: 11,
      width: `${pct}%`,
      background: 'var(--track-fill)',
      borderRadius: 'var(--radius-pill)'
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: `calc(${pct}% - 12px)`,
      width: 24,
      height: 24,
      borderRadius: '50%',
      background: 'var(--ink-050)',
      boxShadow: 'var(--shadow-panel)'
    }
  }), /*#__PURE__*/React.createElement("input", {
    type: "range",
    value: value,
    min: min,
    max: max,
    step: step,
    disabled: disabled,
    onChange: e => onChange && onChange(Number(e.target.value)),
    style: {
      position: 'absolute',
      inset: 0,
      width: '100%',
      opacity: 0,
      margin: 0,
      cursor: disabled ? 'not-allowed' : 'pointer'
    }
  })));
}
Object.assign(__ds_scope, { Slider });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/Slider.jsx", error: String((e && e.message) || e) }); }

// components/forms/Switch.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Binary hardware or preference toggle. Applies immediately — no save step. */
function Switch({
  checked,
  onChange,
  label,
  description,
  disabled,
  style,
  ...rest
}) {
  return /*#__PURE__*/React.createElement("label", _extends({}, rest, {
    style: {
      display: 'flex',
      alignItems: 'center',
      gap: 'var(--space-5)',
      minHeight: 'var(--hit-min)',
      cursor: disabled ? 'not-allowed' : 'pointer',
      opacity: disabled ? .5 : 1,
      ...style
    }
  }), /*#__PURE__*/React.createElement("span", {
    style: {
      flex: 1,
      minWidth: 0
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      display: 'block',
      fontSize: 'var(--type-body)',
      color: 'var(--text-primary)'
    }
  }, label), description && /*#__PURE__*/React.createElement("span", {
    style: {
      display: 'block',
      fontSize: 'var(--type-caption)',
      color: 'var(--text-tertiary)',
      lineHeight: 'var(--leading-body)'
    }
  }, description)), /*#__PURE__*/React.createElement("span", {
    role: "switch",
    "aria-checked": !!checked,
    tabIndex: 0,
    onClick: () => !disabled && onChange && onChange(!checked),
    style: {
      position: 'relative',
      flex: '0 0 auto',
      width: 52,
      height: 30,
      background: checked ? 'var(--current-500)' : 'var(--ink-700)',
      border: `var(--border-default) solid ${checked ? 'transparent' : 'var(--line-default)'}`,
      borderRadius: 'var(--radius-pill)',
      transition: 'background var(--dur-fast) var(--ease-standard)'
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      position: 'absolute',
      top: 3,
      left: checked ? 25 : 3,
      width: 22,
      height: 22,
      borderRadius: '50%',
      background: checked ? 'var(--ink-950)' : 'var(--ink-300)',
      transition: 'left var(--dur-fast) var(--ease-standard)'
    }
  })));
}
Object.assign(__ds_scope, { Switch });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/Switch.jsx", error: String((e && e.message) || e) }); }

// components/forms/TextField.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Text input for pairing codes, bike names, account fields. Companion app only. */
function TextField({
  label,
  hint,
  invalid,
  mono,
  style,
  ...rest
}) {
  return /*#__PURE__*/React.createElement("label", {
    style: {
      display: 'block',
      ...style
    }
  }, label && /*#__PURE__*/React.createElement("span", {
    style: {
      display: 'block',
      marginBottom: 'var(--space-3)',
      fontSize: 'var(--type-label)',
      letterSpacing: 'var(--tracking-label)',
      textTransform: 'uppercase',
      color: 'var(--text-secondary)'
    }
  }, label), /*#__PURE__*/React.createElement("input", _extends({}, rest, {
    style: {
      width: '100%',
      height: 'var(--hit-min)',
      padding: '0 var(--space-4)',
      background: 'var(--surface-sunken)',
      color: 'var(--text-primary)',
      border: `var(--border-default) solid ${invalid ? 'var(--signal-critical)' : 'var(--line-default)'}`,
      borderRadius: 'var(--radius-control)',
      fontFamily: mono ? 'var(--font-mono)' : 'var(--font-core)',
      fontSize: 'var(--type-body)',
      letterSpacing: mono ? '0.12em' : 'normal'
    }
  })), hint && /*#__PURE__*/React.createElement("span", {
    style: {
      display: 'block',
      marginTop: 'var(--space-2)',
      fontSize: 'var(--type-caption)',
      color: invalid ? 'var(--signal-critical)' : 'var(--text-tertiary)'
    }
  }, hint));
}
Object.assign(__ds_scope, { TextField });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/TextField.jsx", error: String((e && e.message) || e) }); }

// ui_kits/cluster/ClusterApp.jsx
try { (() => {
const {
  TelltaleRail,
  SegmentedControl,
  Badge,
  Icon,
  Toast
} = window.VoltlineClusterDesignSystem_122293;
function ClusterApp() {
  const [view, setView] = React.useState('ride');
  const [mode, setMode] = React.useState('city');
  const [theme, setTheme] = React.useState('night');
  const [units, setUnits] = React.useState('metric');
  const [fault, setFault] = React.useState(true);
  const [turn, setTurn] = React.useState('left');
  const [speed, setSpeed] = React.useState(27);
  const [watts, setWatts] = React.useState(-120);
  const [toast, setToast] = React.useState(null);
  React.useEffect(() => {
    const t = setInterval(() => {
      setSpeed(s => Math.max(0, Math.min(44, s + (Math.random() * 6 - 3))));
      setWatts(w => Math.max(-300, Math.min(740, w + (Math.random() * 240 - 110))));
    }, 1400);
    return () => clearInterval(t);
  }, []);
  const showToast = msg => {
    setToast(msg);
    setTimeout(() => setToast(null), 2400);
  };
  const onMode = m => {
    setMode(m);
    showToast(`${m[0].toUpperCase() + m.slice(1)} mode`);
  };
  const rail = [{
    icon: 'chevron-left',
    tone: 'turn',
    active: turn === 'left',
    blink: true,
    label: 'Left indicator'
  }, {
    icon: 'lightbulb',
    tone: 'beam',
    active: true,
    label: 'High beam'
  }, {
    icon: 'battery-warning',
    tone: 'caution',
    active: false,
    label: 'Low charge'
  }, {
    icon: 'octagon-alert',
    tone: 'critical',
    active: fault,
    label: 'Fault'
  }, {
    icon: 'thermometer',
    tone: 'caution',
    active: fault,
    label: 'Motor temperature'
  }, {
    icon: 'bluetooth',
    tone: 'info',
    active: true,
    label: 'Phone connected'
  }, {
    icon: 'lock',
    tone: 'info',
    active: false,
    label: 'Immobiliser'
  }, {
    icon: 'chevron-right',
    tone: 'turn',
    active: turn === 'right',
    blink: true,
    label: 'Right indicator'
  }];
  return /*#__PURE__*/React.createElement("div", {
    "data-theme": theme === 'day' ? 'day' : undefined,
    style: {
      position: 'relative',
      width: 1280,
      height: 800,
      display: 'flex',
      flexDirection: 'column',
      background: 'var(--bg-void)',
      color: 'var(--text-primary)',
      borderRadius: 'var(--radius-screen)',
      overflow: 'hidden'
    }
  }, /*#__PURE__*/React.createElement("header", {
    style: {
      display: 'flex',
      alignItems: 'center',
      gap: 'var(--space-6)',
      padding: '0 var(--gutter-cluster)',
      height: 'var(--bar-status)',
      background: 'var(--surface-sunken)',
      borderBottom: '1px solid var(--line-hairline)'
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: 'var(--font-mono)',
      fontSize: 'var(--type-caption)',
      color: 'var(--text-tertiary)'
    }
  }, "07:54"), /*#__PURE__*/React.createElement(TelltaleRail, {
    items: rail,
    size: 26,
    style: {
      flex: 1
    }
  }), /*#__PURE__*/React.createElement("span", {
    style: {
      display: 'flex',
      alignItems: 'center',
      gap: 6,
      fontFamily: 'var(--font-mono)',
      fontSize: 'var(--type-caption)',
      color: 'var(--text-tertiary)'
    }
  }, /*#__PURE__*/React.createElement(Icon, {
    name: "thermometer",
    size: 13
  }), "14\xB0C")), view === 'ride' && /*#__PURE__*/React.createElement(RideScreen, {
    speed: speed,
    mode: mode,
    setMode: onMode,
    watts: watts,
    soc: 62
  }), view === 'trip' && /*#__PURE__*/React.createElement(TripScreen, null), view === 'nav' && /*#__PURE__*/React.createElement(NavScreen, null), view === 'diag' && /*#__PURE__*/React.createElement(DiagnosticsScreen, {
    fault: fault,
    clearFault: () => {
      setFault(false);
      showToast('Fault acknowledged');
    }
  }), view === 'set' && /*#__PURE__*/React.createElement(SettingsScreen, {
    theme: theme,
    setTheme: setTheme,
    units: units,
    setUnits: setUnits
  }), /*#__PURE__*/React.createElement("footer", {
    style: {
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'space-between',
      gap: 'var(--space-6)',
      padding: '0 var(--gutter-cluster)',
      height: 'var(--bar-tab)',
      background: 'var(--surface-sunken)',
      borderTop: '1px solid var(--line-hairline)'
    }
  }, /*#__PURE__*/React.createElement(SegmentedControl, {
    size: "lg",
    value: view,
    onChange: setView,
    options: [{
      value: 'ride',
      label: 'Ride',
      icon: 'gauge'
    }, {
      value: 'trip',
      label: 'Trip',
      icon: 'route'
    }, {
      value: 'nav',
      label: 'Nav',
      icon: 'map'
    }, {
      value: 'diag',
      label: 'Health',
      icon: 'stethoscope'
    }, {
      value: 'set',
      label: 'Setup',
      icon: 'settings'
    }]
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      alignItems: 'center',
      gap: 'var(--space-3)'
    }
  }, /*#__PURE__*/React.createElement(Badge, {
    tone: "accent"
  }, mode.toUpperCase()), /*#__PURE__*/React.createElement("button", {
    onClick: () => setTurn(turn === 'left' ? 'right' : turn === 'right' ? null : 'left'),
    style: {
      height: 44,
      padding: '0 16px',
      background: 'transparent',
      border: '1px solid var(--line-default)',
      borderRadius: 'var(--radius-control)',
      color: 'var(--text-tertiary)',
      fontFamily: 'var(--font-core)',
      fontSize: 'var(--type-caption)',
      letterSpacing: 'var(--tracking-label)',
      textTransform: 'uppercase',
      cursor: 'pointer'
    }
  }, "Indicator"))), toast && /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      left: '50%',
      bottom: 96,
      transform: 'translateX(-50%)'
    }
  }, /*#__PURE__*/React.createElement(Toast, {
    icon: "check",
    tone: "go"
  }, toast)));
}
Object.assign(window, {
  ClusterApp
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/cluster/ClusterApp.jsx", error: String((e && e.message) || e) }); }

// ui_kits/cluster/DiagnosticsScreen.jsx
try { (() => {
const {
  Panel,
  AlertBanner,
  TripStat,
  Button,
  Badge
} = window.VoltlineClusterDesignSystem_122293;
function DiagnosticsScreen({
  fault,
  clearFault
}) {
  return /*#__PURE__*/React.createElement("div", {
    style: {
      flex: 1,
      display: 'grid',
      gridTemplateColumns: '1fr 320px',
      gap: 'var(--gap-tile)',
      padding: 'var(--gutter-cluster)',
      minHeight: 0
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--gap-tile)',
      alignContent: 'start'
    }
  }, fault && /*#__PURE__*/React.createElement(AlertBanner, {
    tone: "critical",
    title: "Motor over temperature",
    detail: "Power limited to 60%. Pull over when safe and let the motor cool.",
    code: "E-412",
    action: /*#__PURE__*/React.createElement(Button, {
      size: "sm",
      tone: "secondary",
      onClick: clearFault
    }, "Acknowledge")
  }), /*#__PURE__*/React.createElement(AlertBanner, {
    tone: "caution",
    title: "Front brake pads at 18%",
    detail: "Book service within 300 km.",
    code: "M-020"
  }), /*#__PURE__*/React.createElement(Panel, {
    title: "System health"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gridTemplateColumns: 'repeat(3, 1fr)',
      gap: 'var(--space-7)'
    }
  }, /*#__PURE__*/React.createElement(TripStat, {
    label: "Pack voltage",
    value: "52.4",
    unit: "V",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Pack temp",
    value: "28",
    unit: "\xB0C",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Motor temp",
    value: fault ? '104' : '41',
    unit: "\xB0C",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Controller",
    value: "OK",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Cell delta",
    value: "0.02",
    unit: "V",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Pack health",
    value: "97",
    unit: "%",
    size: "sm"
  })))), /*#__PURE__*/React.createElement(Panel, {
    title: "Vehicle",
    density: "compact"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--space-5)',
      fontFamily: 'var(--font-mono)',
      fontSize: 'var(--type-caption)',
      color: 'var(--text-secondary)'
    }
  }, /*#__PURE__*/React.createElement("div", null, "MODEL", /*#__PURE__*/React.createElement("br", null), /*#__PURE__*/React.createElement("span", {
    style: {
      color: 'var(--text-primary)'
    }
  }, "VL-M2 Roadster")), /*#__PURE__*/React.createElement("div", null, "SERIAL", /*#__PURE__*/React.createElement("br", null), /*#__PURE__*/React.createElement("span", {
    style: {
      color: 'var(--text-primary)'
    }
  }, "9F3A-2201-88")), /*#__PURE__*/React.createElement("div", null, "FIRMWARE", /*#__PURE__*/React.createElement("br", null), /*#__PURE__*/React.createElement("span", {
    style: {
      color: 'var(--text-primary)'
    }
  }, "2.8.1 (cluster) \xB7 1.4.0 (bms)")), /*#__PURE__*/React.createElement("div", null, "LAST SERVICE", /*#__PURE__*/React.createElement("br", null), /*#__PURE__*/React.createElement("span", {
    style: {
      color: 'var(--text-primary)'
    }
  }, "2026-04-11 \xB7 3 902 km"))), /*#__PURE__*/React.createElement("div", {
    style: {
      marginTop: 'var(--space-6)'
    }
  }, /*#__PURE__*/React.createElement(Badge, {
    tone: fault ? 'critical' : 'go',
    icon: fault ? 'octagon-alert' : 'check'
  }, fault ? '1 active fault' : 'No active faults'))));
}
Object.assign(window, {
  DiagnosticsScreen
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/cluster/DiagnosticsScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/cluster/NavScreen.jsx
try { (() => {
const {
  Panel,
  TripStat,
  Icon,
  Badge
} = window.VoltlineClusterDesignSystem_122293;

/* Map tiles are supplied by the host device SDK — this kit shows the chrome only. */
function NavScreen() {
  return /*#__PURE__*/React.createElement("div", {
    style: {
      flex: 1,
      display: 'grid',
      gridTemplateColumns: '360px 1fr',
      gap: 'var(--gap-tile)',
      padding: 'var(--gutter-cluster)',
      minHeight: 0
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--gap-tile)',
      alignContent: 'start'
    }
  }, /*#__PURE__*/React.createElement(Panel, {
    tone: "accent",
    density: "comfortable"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      alignItems: 'center',
      gap: 'var(--space-5)'
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      color: 'var(--current-500)',
      display: 'flex'
    }
  }, /*#__PURE__*/React.createElement(Icon, {
    name: "corner-up-right",
    size: 56
  })), /*#__PURE__*/React.createElement("div", null, /*#__PURE__*/React.createElement("div", {
    style: {
      fontFamily: 'var(--font-readout)',
      fontSize: 'var(--type-readout-2)',
      fontWeight: 600,
      lineHeight: 1,
      fontVariantNumeric: 'tabular-nums'
    }
  }, "240", /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--type-label)',
      color: 'var(--text-tertiary)'
    }
  }, " m")), /*#__PURE__*/React.createElement("div", {
    style: {
      fontSize: 'var(--type-body-lg)',
      color: 'var(--text-primary)'
    }
  }, "Turn right onto Kanaalweg")))), /*#__PURE__*/React.createElement(Panel, {
    title: "Then",
    density: "compact"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      alignItems: 'center',
      gap: 'var(--space-4)',
      color: 'var(--text-secondary)'
    }
  }, /*#__PURE__*/React.createElement(Icon, {
    name: "corner-up-left",
    size: 24
  }), /*#__PURE__*/React.createElement("span", null, "1.4 km \xB7 Left onto Havenstraat"))), /*#__PURE__*/React.createElement(Panel, {
    title: "Destination",
    meta: "eta 08:14",
    density: "compact"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--space-5)'
    }
  }, /*#__PURE__*/React.createElement(TripStat, {
    label: "Remaining",
    value: "6.8",
    unit: "km",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Arrival charge",
    value: "41",
    unit: "%",
    size: "sm",
    tone: "accent"
  })))), /*#__PURE__*/React.createElement("div", {
    style: {
      borderRadius: 'var(--radius-panel)',
      border: '1px dashed var(--line-default)',
      background: 'var(--surface-sunken)',
      display: 'grid',
      placeItems: 'center',
      gap: 'var(--space-4)'
    }
  }, /*#__PURE__*/React.createElement(Badge, {
    icon: "map"
  }, "Map render area"), /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--type-caption)',
      color: 'var(--text-tertiary)',
      maxWidth: 320,
      textAlign: 'center'
    }
  }, "Tiles are drawn by the host navigation SDK. No map style is defined in this design system yet.")));
}
Object.assign(window, {
  NavScreen
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/cluster/NavScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/cluster/RideScreen.jsx
try { (() => {
const {
  SpeedReadout,
  ArcGauge,
  BatteryGauge,
  PowerFlowBar,
  TripStat,
  RideModeSelector,
  Panel,
  Badge
} = window.VoltlineClusterDesignSystem_122293;
function RideScreen({
  speed,
  mode,
  setMode,
  watts,
  soc
}) {
  return /*#__PURE__*/React.createElement("div", {
    style: {
      flex: 1,
      display: 'grid',
      gridTemplateColumns: '300px 1fr 300px',
      gap: 'var(--gap-tile)',
      padding: 'var(--gutter-cluster)',
      minHeight: 0
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--gap-tile)',
      alignContent: 'start'
    }
  }, /*#__PURE__*/React.createElement(Panel, {
    title: "Battery",
    meta: "52.4 V",
    density: "compact"
  }, /*#__PURE__*/React.createElement(BatteryGauge, {
    percent: soc,
    rangeKm: Math.round(soc * 0.78)
  })), /*#__PURE__*/React.createElement(Panel, {
    title: "Motor",
    density: "compact"
  }, /*#__PURE__*/React.createElement(PowerFlowBar, {
    watts: watts
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      justifyContent: 'space-between',
      marginTop: 'var(--space-5)'
    }
  }, /*#__PURE__*/React.createElement(TripStat, {
    label: "Motor",
    value: "41",
    unit: "\xB0C",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Pack",
    value: "28",
    unit: "\xB0C",
    size: "sm"
  }))), /*#__PURE__*/React.createElement(RideModeSelector, {
    orientation: "vertical",
    value: mode,
    onChange: setMode,
    modes: [{
      value: 'eco',
      label: 'Eco',
      icon: 'leaf'
    }, {
      value: 'city',
      label: 'City',
      icon: 'building-2'
    }, {
      value: 'sport',
      label: 'Sport',
      icon: 'zap'
    }]
  })), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      placeItems: 'center'
    }
  }, /*#__PURE__*/React.createElement(ArcGauge, {
    value: speed,
    max: 45,
    size: 430,
    thickness: 16,
    tone: speed > 32 ? 'caution' : 'accent'
  }, /*#__PURE__*/React.createElement(SpeedReadout, {
    value: speed,
    size: "xl",
    unit: "km/h",
    limit: 32
  }))), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--gap-tile)',
      alignContent: 'start'
    }
  }, /*#__PURE__*/React.createElement(Panel, {
    title: "Trip A",
    meta: "07:12",
    density: "compact"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--space-5)'
    }
  }, /*#__PURE__*/React.createElement(TripStat, {
    icon: "route",
    label: "Distance",
    value: "18.4",
    unit: "km"
  }), /*#__PURE__*/React.createElement(TripStat, {
    icon: "timer",
    label: "Moving",
    value: "42:07"
  }), /*#__PURE__*/React.createElement(TripStat, {
    icon: "zap",
    label: "Avg use",
    value: "9.2",
    unit: "Wh/km",
    tone: "accent"
  }))), /*#__PURE__*/React.createElement(Panel, {
    title: "Odometer",
    density: "compact"
  }, /*#__PURE__*/React.createElement(TripStat, {
    label: "Total",
    value: "4 218",
    unit: "km",
    size: "sm"
  })), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      gap: 'var(--space-2)',
      flexWrap: 'wrap'
    }
  }, /*#__PURE__*/React.createElement(Badge, {
    tone: "go",
    icon: "check"
  }, "Ready"), /*#__PURE__*/React.createElement(Badge, {
    tone: "accent",
    icon: "bluetooth"
  }, "Phone"))));
}
Object.assign(window, {
  RideScreen
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/cluster/RideScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/cluster/SettingsScreen.jsx
try { (() => {
const {
  Panel,
  Switch,
  Slider,
  SegmentedControl,
  Button
} = window.VoltlineClusterDesignSystem_122293;
function SettingsScreen({
  theme,
  setTheme,
  units,
  setUnits
}) {
  const [bright, setBright] = React.useState(72);
  const [regen, setRegen] = React.useState(2);
  const [lights, setLights] = React.useState(true);
  const [alarm, setAlarm] = React.useState(false);
  return /*#__PURE__*/React.createElement("div", {
    style: {
      flex: 1,
      display: 'grid',
      gridTemplateColumns: '1fr 1fr',
      gap: 'var(--gap-tile)',
      padding: 'var(--gutter-cluster)',
      minHeight: 0
    }
  }, /*#__PURE__*/React.createElement(Panel, {
    title: "Display"
  }, /*#__PURE__*/React.createElement(Slider, {
    label: "Brightness",
    unit: "%",
    value: bright,
    onChange: setBright
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      height: 'var(--space-7)'
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--space-3)'
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--type-label)',
      letterSpacing: 'var(--tracking-label)',
      textTransform: 'uppercase',
      color: 'var(--text-secondary)'
    }
  }, "Theme"), /*#__PURE__*/React.createElement(SegmentedControl, {
    block: true,
    value: theme,
    onChange: setTheme,
    options: [{
      value: 'night',
      label: 'Night',
      icon: 'moon'
    }, {
      value: 'day',
      label: 'Day',
      icon: 'sun'
    }]
  })), /*#__PURE__*/React.createElement("div", {
    style: {
      height: 'var(--space-7)'
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--space-3)'
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--type-label)',
      letterSpacing: 'var(--tracking-label)',
      textTransform: 'uppercase',
      color: 'var(--text-secondary)'
    }
  }, "Units"), /*#__PURE__*/React.createElement(SegmentedControl, {
    block: true,
    value: units,
    onChange: setUnits,
    options: [{
      value: 'metric',
      label: 'km / °C'
    }, {
      value: 'imperial',
      label: 'mi / °F'
    }]
  }))), /*#__PURE__*/React.createElement(Panel, {
    title: "Ride"
  }, /*#__PURE__*/React.createElement(Slider, {
    label: "Regen strength",
    value: regen,
    min: 0,
    max: 3,
    onChange: setRegen
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      height: 'var(--space-5)'
    }
  }), /*#__PURE__*/React.createElement(Switch, {
    label: "Auto headlight",
    description: "Turns on below 40 lux",
    checked: lights,
    onChange: setLights
  }), /*#__PURE__*/React.createElement(Switch, {
    label: "Theft alarm",
    description: "Alerts your phone if the bike moves while locked",
    checked: alarm,
    onChange: setAlarm
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      gap: 'var(--space-3)',
      marginTop: 'var(--space-7)'
    }
  }, /*#__PURE__*/React.createElement(Button, {
    tone: "secondary",
    icon: "smartphone"
  }, "Pair a phone"), /*#__PURE__*/React.createElement(Button, {
    tone: "ghost",
    icon: "download"
  }, "Check for updates"))));
}
Object.assign(window, {
  SettingsScreen
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/cluster/SettingsScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/cluster/TripScreen.jsx
try { (() => {
const {
  Panel,
  TripStat,
  SegmentedControl,
  Button,
  BatteryGauge
} = window.VoltlineClusterDesignSystem_122293;
function TripScreen() {
  const [scope, setScope] = React.useState('a');
  const rows = scope === 'a' ? [['Distance', '18.4', 'km'], ['Moving time', '42:07', ''], ['Avg speed', '26.2', 'km/h'], ['Top speed', '41.8', 'km/h'], ['Energy used', '169', 'Wh'], ['Efficiency', '9.2', 'Wh/km'], ['Regen recovered', '14', 'Wh'], ['Ascent', '142', 'm']] : [['Distance', '311.6', 'km'], ['Moving time', '11:48:20', ''], ['Avg speed', '24.4', 'km/h'], ['Top speed', '44.1', 'km/h'], ['Energy used', '2 940', 'Wh'], ['Efficiency', '9.4', 'Wh/km'], ['Regen recovered', '206', 'Wh'], ['Ascent', '2 815', 'm']];
  return /*#__PURE__*/React.createElement("div", {
    style: {
      flex: 1,
      display: 'grid',
      gridTemplateColumns: '1fr 320px',
      gap: 'var(--gap-tile)',
      padding: 'var(--gutter-cluster)',
      minHeight: 0
    }
  }, /*#__PURE__*/React.createElement(Panel, {
    title: "Ride statistics",
    meta: scope === 'a' ? 'trip a' : 'trip b'
  }, /*#__PURE__*/React.createElement(SegmentedControl, {
    value: scope,
    onChange: setScope,
    options: [{
      value: 'a',
      label: 'Trip A'
    }, {
      value: 'b',
      label: 'Trip B'
    }]
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gridTemplateColumns: 'repeat(4, 1fr)',
      gap: 'var(--space-8) var(--space-7)',
      marginTop: 'var(--space-8)'
    }
  }, rows.map(([l, v, u]) => /*#__PURE__*/React.createElement(TripStat, {
    key: l,
    label: l,
    value: v,
    unit: u
  }))), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      gap: 'var(--space-3)',
      marginTop: 'var(--space-9)'
    }
  }, /*#__PURE__*/React.createElement(Button, {
    tone: "secondary",
    icon: "rotate-ccw"
  }, "Reset ", scope === 'a' ? 'Trip A' : 'Trip B'), /*#__PURE__*/React.createElement(Button, {
    tone: "ghost",
    icon: "cloud-upload"
  }, "Sync to phone"))), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--gap-tile)',
      alignContent: 'start'
    }
  }, /*#__PURE__*/React.createElement(Panel, {
    title: "Battery",
    density: "compact"
  }, /*#__PURE__*/React.createElement(BatteryGauge, {
    percent: 62,
    rangeKm: 48
  })), /*#__PURE__*/React.createElement(Panel, {
    title: "Since charge",
    density: "compact"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--space-5)'
    }
  }, /*#__PURE__*/React.createElement(TripStat, {
    label: "Distance",
    value: "62.9",
    unit: "km",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Cycles",
    value: "184",
    size: "sm"
  })))));
}
Object.assign(window, {
  TripScreen
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/cluster/TripScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/companion-app/BikeScreen.jsx
try { (() => {
const {
  Panel,
  BatteryGauge,
  TripStat,
  Badge,
  Button,
  RideModeSelector,
  Icon,
  IconButton
} = window.VoltlineClusterDesignSystem_122293;
function BikeScreen({
  locked,
  setLocked,
  mode,
  setMode
}) {
  return /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--gap-tile)'
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'space-between'
    }
  }, /*#__PURE__*/React.createElement("div", null, /*#__PURE__*/React.createElement("div", {
    style: {
      fontSize: 'var(--type-title)',
      fontWeight: 600,
      lineHeight: 1.1
    }
  }, "Roadster"), /*#__PURE__*/React.createElement("div", {
    style: {
      fontSize: 'var(--type-caption)',
      color: 'var(--text-tertiary)'
    }
  }, "VL-M2 \xB7 last seen 2 min ago")), /*#__PURE__*/React.createElement(Badge, {
    tone: locked ? 'go' : 'caution',
    icon: locked ? 'lock' : 'lock-open'
  }, locked ? 'Locked' : 'Unlocked')), /*#__PURE__*/React.createElement(Panel, {
    title: "Charge",
    meta: "est. full 09:20"
  }, /*#__PURE__*/React.createElement(BatteryGauge, {
    percent: 62,
    rangeKm: 48,
    charging: true
  })), /*#__PURE__*/React.createElement(Panel, {
    title: "Assist mode"
  }, /*#__PURE__*/React.createElement(RideModeSelector, {
    value: mode,
    onChange: setMode,
    modes: [{
      value: 'eco',
      label: 'Eco',
      icon: 'leaf'
    }, {
      value: 'city',
      label: 'City',
      icon: 'building-2'
    }, {
      value: 'sport',
      label: 'Sport',
      icon: 'zap'
    }]
  })), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gridTemplateColumns: '1fr 1fr',
      gap: 'var(--gap-tile)'
    }
  }, /*#__PURE__*/React.createElement(Panel, {
    density: "compact"
  }, /*#__PURE__*/React.createElement(TripStat, {
    icon: "route",
    label: "This week",
    value: "86.2",
    unit: "km",
    size: "sm"
  })), /*#__PURE__*/React.createElement(Panel, {
    density: "compact"
  }, /*#__PURE__*/React.createElement(TripStat, {
    icon: "zap",
    label: "Avg use",
    value: "9.4",
    unit: "Wh/km",
    size: "sm",
    tone: "accent"
  }))), /*#__PURE__*/React.createElement(Panel, {
    title: "Where it is",
    density: "compact"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      alignItems: 'center',
      gap: 'var(--space-4)',
      color: 'var(--text-secondary)',
      fontSize: 'var(--type-body)'
    }
  }, /*#__PURE__*/React.createElement(Icon, {
    name: "map-pin",
    size: 20
  }), /*#__PURE__*/React.createElement("span", null, "Kanaalweg 22, Utrecht"))), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      gap: 'var(--space-3)'
    }
  }, /*#__PURE__*/React.createElement(Button, {
    block: true,
    tone: locked ? 'primary' : 'secondary',
    size: "lg",
    icon: locked ? 'lock-open' : 'lock',
    onClick: () => setLocked(!locked)
  }, locked ? 'Unlock' : 'Lock'), /*#__PURE__*/React.createElement(IconButton, {
    size: "lg",
    icon: "bell-ring",
    label: "Ring bike"
  })));
}
Object.assign(window, {
  BikeScreen
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/companion-app/BikeScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/companion-app/CompanionApp.jsx
try { (() => {
const {
  SegmentedControl,
  Icon
} = window.VoltlineClusterDesignSystem_122293;
function CompanionApp() {
  const [tab, setTab] = React.useState('bike');
  const [ride, setRide] = React.useState(null);
  const [locked, setLocked] = React.useState(true);
  const [mode, setMode] = React.useState('city');
  const TABS = [{
    value: 'bike',
    label: 'Bike',
    icon: 'bike'
  }, {
    value: 'rides',
    label: 'Rides',
    icon: 'route'
  }, {
    value: 'setup',
    label: 'Setup',
    icon: 'settings'
  }];
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'relative',
      width: 390,
      height: 844,
      display: 'flex',
      flexDirection: 'column',
      background: 'var(--surface-base)',
      color: 'var(--text-primary)',
      borderRadius: 'var(--radius-screen)',
      overflow: 'hidden',
      border: '1px solid var(--line-hairline)'
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'space-between',
      padding: '0 var(--gutter-app)',
      height: 'var(--bar-status)',
      fontFamily: 'var(--font-mono)',
      fontSize: 'var(--type-caption)',
      color: 'var(--text-tertiary)'
    }
  }, /*#__PURE__*/React.createElement("span", null, "07:54"), /*#__PURE__*/React.createElement("span", {
    style: {
      display: 'flex',
      gap: 8,
      alignItems: 'center'
    }
  }, /*#__PURE__*/React.createElement(Icon, {
    name: "signal",
    size: 13
  }), /*#__PURE__*/React.createElement(Icon, {
    name: "wifi",
    size: 13
  }), /*#__PURE__*/React.createElement(Icon, {
    name: "battery-full",
    size: 15
  }))), /*#__PURE__*/React.createElement("main", {
    style: {
      flex: 1,
      overflowY: 'auto',
      padding: '0 var(--gutter-app) var(--space-8)'
    }
  }, tab === 'bike' && /*#__PURE__*/React.createElement(BikeScreen, {
    locked: locked,
    setLocked: setLocked,
    mode: mode,
    setMode: setMode
  }), tab === 'rides' && (ride ? /*#__PURE__*/React.createElement(RideDetailScreen, {
    ride: ride,
    onBack: () => setRide(null)
  }) : /*#__PURE__*/React.createElement(RidesScreen, {
    onOpen: setRide
  })), tab === 'setup' && /*#__PURE__*/React.createElement(SetupScreen, null)), /*#__PURE__*/React.createElement("nav", {
    style: {
      padding: 'var(--space-3) var(--gutter-app) var(--space-6)',
      borderTop: '1px solid var(--line-hairline)',
      background: 'var(--surface-sunken)'
    }
  }, /*#__PURE__*/React.createElement(SegmentedControl, {
    block: true,
    size: "lg",
    value: tab,
    onChange: v => {
      setTab(v);
      setRide(null);
    },
    options: TABS
  })));
}
Object.assign(window, {
  CompanionApp
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/companion-app/CompanionApp.jsx", error: String((e && e.message) || e) }); }

// ui_kits/companion-app/RideDetailScreen.jsx
try { (() => {
const {
  Panel,
  TripStat,
  Button,
  Badge,
  Icon,
  Dialog
} = window.VoltlineClusterDesignSystem_122293;
function RideDetailScreen({
  ride,
  onBack
}) {
  const [confirm, setConfirm] = React.useState(false);
  return /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--gap-tile)',
      position: 'relative'
    }
  }, /*#__PURE__*/React.createElement("button", {
    onClick: onBack,
    style: {
      display: 'inline-flex',
      alignItems: 'center',
      gap: 6,
      background: 'none',
      border: 0,
      padding: 0,
      color: 'var(--text-secondary)',
      fontFamily: 'var(--font-core)',
      fontSize: 'var(--type-label)',
      cursor: 'pointer'
    }
  }, /*#__PURE__*/React.createElement(Icon, {
    name: "chevron-left",
    size: 16
  }), " Rides"), /*#__PURE__*/React.createElement("div", null, /*#__PURE__*/React.createElement("div", {
    style: {
      fontSize: 'var(--type-title)',
      fontWeight: 600
    }
  }, ride.title), /*#__PURE__*/React.createElement("div", {
    style: {
      fontSize: 'var(--type-caption)',
      color: 'var(--text-tertiary)'
    }
  }, ride.when)), /*#__PURE__*/React.createElement("div", {
    style: {
      height: 140,
      borderRadius: 'var(--radius-tile)',
      border: '1px dashed var(--line-default)',
      background: 'var(--surface-sunken)',
      display: 'grid',
      placeItems: 'center'
    }
  }, /*#__PURE__*/React.createElement(Badge, {
    icon: "map"
  }, "Route map \u2014 host SDK")), /*#__PURE__*/React.createElement(Panel, {
    title: "Summary"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gridTemplateColumns: '1fr 1fr',
      gap: 'var(--space-7)'
    }
  }, /*#__PURE__*/React.createElement(TripStat, {
    label: "Distance",
    value: ride.km,
    unit: "km",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Moving",
    value: `${ride.min} min`,
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Avg speed",
    value: "26.2",
    unit: "km/h",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Top speed",
    value: "41.8",
    unit: "km/h",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Energy",
    value: ride.wh,
    unit: "Wh",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Regen",
    value: "14",
    unit: "Wh",
    size: "sm",
    tone: "accent"
  }))), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      gap: 'var(--space-3)'
    }
  }, /*#__PURE__*/React.createElement(Button, {
    tone: "secondary",
    icon: "share-2",
    block: true
  }, "Share"), /*#__PURE__*/React.createElement(Button, {
    tone: "ghost",
    icon: "trash-2",
    onClick: () => setConfirm(true)
  }, "Delete")), /*#__PURE__*/React.createElement(Dialog, {
    open: confirm,
    width: 300,
    title: "Discard this ride?",
    actions: /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement(Button, {
      size: "sm",
      tone: "ghost",
      onClick: () => setConfirm(false)
    }, "Keep"), /*#__PURE__*/React.createElement(Button, {
      size: "sm",
      tone: "critical",
      onClick: () => {
        setConfirm(false);
        onBack();
      }
    }, "Discard"))
  }, ride.km, " km and ", ride.min, " minutes will be removed."));
}
Object.assign(window, {
  RideDetailScreen
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/companion-app/RideDetailScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/companion-app/RidesScreen.jsx
try { (() => {
const {
  Panel,
  TripStat,
  SegmentedControl,
  Icon
} = window.VoltlineClusterDesignSystem_122293;
const RIDES = [{
  id: 1,
  title: 'Morning commute',
  when: 'Today · 07:12',
  km: '18.4',
  min: '42',
  wh: '169'
}, {
  id: 2,
  title: 'Groceries',
  when: 'Yesterday · 18:40',
  km: '5.2',
  min: '14',
  wh: '48'
}, {
  id: 3,
  title: 'Canal loop',
  when: 'Sun · 10:02',
  km: '41.7',
  min: '96',
  wh: '392'
}, {
  id: 4,
  title: 'Evening commute',
  when: 'Fri · 17:55',
  km: '18.1',
  min: '39',
  wh: '171'
}];
function RidesScreen({
  onOpen
}) {
  const [range, setRange] = React.useState('week');
  return /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--gap-tile)'
    }
  }, /*#__PURE__*/React.createElement(SegmentedControl, {
    block: true,
    value: range,
    onChange: setRange,
    options: [{
      value: 'week',
      label: 'Week'
    }, {
      value: 'month',
      label: 'Month'
    }, {
      value: 'all',
      label: 'All'
    }]
  }), /*#__PURE__*/React.createElement(Panel, {
    density: "compact"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      justifyContent: 'space-between'
    }
  }, /*#__PURE__*/React.createElement(TripStat, {
    label: "Distance",
    value: "86.2",
    unit: "km",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Rides",
    value: "7",
    size: "sm"
  }), /*#__PURE__*/React.createElement(TripStat, {
    label: "Energy",
    value: "810",
    unit: "Wh",
    size: "sm"
  }))), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--space-3)'
    }
  }, RIDES.map(r => /*#__PURE__*/React.createElement("button", {
    key: r.id,
    onClick: () => onOpen(r),
    style: {
      display: 'flex',
      alignItems: 'center',
      gap: 'var(--space-4)',
      width: '100%',
      padding: 'var(--space-4) var(--space-5)',
      background: 'var(--surface-panel)',
      border: '1px solid var(--line-hairline)',
      borderRadius: 'var(--radius-tile)',
      cursor: 'pointer',
      textAlign: 'left',
      color: 'var(--text-primary)',
      fontFamily: 'var(--font-core)'
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      color: 'var(--current-500)',
      display: 'flex'
    }
  }, /*#__PURE__*/React.createElement(Icon, {
    name: "route",
    size: 20
  })), /*#__PURE__*/React.createElement("span", {
    style: {
      flex: 1,
      minWidth: 0
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      display: 'block',
      fontSize: 'var(--type-body)'
    }
  }, r.title), /*#__PURE__*/React.createElement("span", {
    style: {
      display: 'block',
      fontSize: 'var(--type-caption)',
      color: 'var(--text-tertiary)'
    }
  }, r.when)), /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: 'var(--font-readout)',
      fontSize: 'var(--type-readout-3)',
      fontWeight: 600,
      lineHeight: 1,
      fontVariantNumeric: 'tabular-nums'
    }
  }, r.km, /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--type-caption)',
      color: 'var(--text-tertiary)'
    }
  }, " km")), /*#__PURE__*/React.createElement(Icon, {
    name: "chevron-right",
    size: 18
  })))));
}
Object.assign(window, {
  RidesScreen
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/companion-app/RidesScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/companion-app/SetupScreen.jsx
try { (() => {
const {
  Panel,
  Switch,
  Slider,
  TextField,
  Button,
  AlertBanner,
  Badge
} = window.VoltlineClusterDesignSystem_122293;
function SetupScreen() {
  const [alarm, setAlarm] = React.useState(true);
  const [auto, setAuto] = React.useState(true);
  const [notify, setNotify] = React.useState(false);
  const [assist, setAssist] = React.useState(3);
  return /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'grid',
      gap: 'var(--gap-tile)'
    }
  }, /*#__PURE__*/React.createElement(AlertBanner, {
    tone: "info",
    title: "Firmware 2.8.1 available",
    detail: "Installs while parked and charging."
  }), /*#__PURE__*/React.createElement(Panel, {
    title: "Bike"
  }, /*#__PURE__*/React.createElement(TextField, {
    label: "Name",
    defaultValue: "Roadster"
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      height: 'var(--space-5)'
    }
  }), /*#__PURE__*/React.createElement(TextField, {
    label: "Pairing code",
    mono: true,
    placeholder: "000000",
    hint: "Six digits shown on the cluster"
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      height: 'var(--space-5)'
    }
  }), /*#__PURE__*/React.createElement(Slider, {
    label: "Default assist",
    value: assist,
    min: 1,
    max: 5,
    onChange: setAssist
  })), /*#__PURE__*/React.createElement(Panel, {
    title: "Security & alerts"
  }, /*#__PURE__*/React.createElement(Switch, {
    label: "Theft alarm",
    description: "Alerts this phone if the bike moves while locked",
    checked: alarm,
    onChange: setAlarm
  }), /*#__PURE__*/React.createElement(Switch, {
    label: "Auto-unlock nearby",
    description: "Unlocks when your phone is within 2 m",
    checked: auto,
    onChange: setAuto
  }), /*#__PURE__*/React.createElement(Switch, {
    label: "Charge complete push",
    checked: notify,
    onChange: setNotify
  })), /*#__PURE__*/React.createElement(Panel, {
    title: "Connected",
    density: "compact"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      gap: 'var(--space-2)',
      flexWrap: 'wrap'
    }
  }, /*#__PURE__*/React.createElement(Badge, {
    tone: "accent",
    icon: "bluetooth"
  }, "Cluster 2.8.1"), /*#__PURE__*/React.createElement(Badge, {
    tone: "go",
    icon: "check"
  }, "BMS 1.4.0"))), /*#__PURE__*/React.createElement(Button, {
    tone: "critical",
    block: true,
    icon: "unlink"
  }, "Unpair this bike"));
}
Object.assign(window, {
  SetupScreen
});
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/companion-app/SetupScreen.jsx", error: String((e && e.message) || e) }); }

__ds_ns.ArcGauge = __ds_scope.ArcGauge;

__ds_ns.BatteryGauge = __ds_scope.BatteryGauge;

__ds_ns.PowerFlowBar = __ds_scope.PowerFlowBar;

__ds_ns.RideModeSelector = __ds_scope.RideModeSelector;

__ds_ns.SpeedReadout = __ds_scope.SpeedReadout;

__ds_ns.Telltale = __ds_scope.Telltale;

__ds_ns.TelltaleRail = __ds_scope.TelltaleRail;

__ds_ns.TripStat = __ds_scope.TripStat;

__ds_ns.Badge = __ds_scope.Badge;

__ds_ns.Button = __ds_scope.Button;

__ds_ns.Icon = __ds_scope.Icon;

__ds_ns.IconButton = __ds_scope.IconButton;

__ds_ns.Panel = __ds_scope.Panel;

__ds_ns.AlertBanner = __ds_scope.AlertBanner;

__ds_ns.Dialog = __ds_scope.Dialog;

__ds_ns.Toast = __ds_scope.Toast;

__ds_ns.SegmentedControl = __ds_scope.SegmentedControl;

__ds_ns.Slider = __ds_scope.Slider;

__ds_ns.Switch = __ds_scope.Switch;

__ds_ns.TextField = __ds_scope.TextField;

})();
