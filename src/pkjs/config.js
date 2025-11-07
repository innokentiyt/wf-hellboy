module.exports = [
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Appearance"
      },
      {
        "type": "toggle",
        "messageKey": "show_am_pm",
        "label": "Show AM/PM",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "show_leading_zero",
        "label": "Show hour leading zero",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "show_weekday",
        "label": "Replace month with weekday",
        "defaultValue": false
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];