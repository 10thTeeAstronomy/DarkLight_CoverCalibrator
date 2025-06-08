---
name: Bug report
about: Create a report to help us improve
title: ''
labels: ''
assignees: iwannabswiss

---

name: Bug Report
description: Report an issue with the DLC firmware, ASCOM app, INDI driver, or other related component
title: "[BUG]: "
labels: ["bug", "triage"]
assignees: ["iwannabswiss"]
body:

  - type: dropdown
    id: application
    attributes:
      label: Application
      description: Which part of the DLC system does this bug relate to?
      options:
        - Firmware
        - INDI Driver
        - ASCOM Driver
        - DLC Windows App
        - Other
    validations:
      required: true

  - type: textarea
    id: describe_bug
    attributes:
      label: Describe the bug
      description: A clear and concise description of what the bug is.
      placeholder: What happened? What was supposed to happen?
    validations:
      required: true

  - type: textarea
    id: reproduce_steps
    attributes:
      label: To Reproduce
      description: Steps to reproduce the behavior
      placeholder: |
        1. Go to '...'
        2. Click on '...'
        3. Scroll down to '...'
        4. See error
    validations:
      required: true

  - type: textarea
    id: expected_behavior
    attributes:
      label: Expected behavior
      description: What did you expect to happen?
      placeholder: A clear and concise description of what you expected to happen.
    validations:
      required: true

  - type: input
    id: os
    attributes:
      label: OS
      description: Operating system used
      placeholder: e.g. Windows 11 Pro
    validations:
      required: true

  - type: input
    id: os_version
    attributes:
      label: OS Version
      placeholder: e.g. 24H2
    validations:
      required: true

  - type: input
    id: driver_platform
    attributes:
      label: Driver Platform
      placeholder: e.g. ASCOM, INDI
    validations:
      required: true

  - type: input
    id: driver_version
    attributes:
      label: Driver Version
      placeholder: e.g. 1.1.0
    validations:
      required: true

  - type: input
    id: firmware_version
    attributes:
      label: Firmware Version
      placeholder: e.g. 1.1.1
    validations:
      required: false

  - type: input
    id: app_version
    attributes:
      label: DLC Windows App Version
      placeholder: e.g. 2.0.0.4
    validations:
      required: false

  - type: textarea
    id: screenshots
    attributes:
      label: Screenshots
      description: Paste a screenshot or drag and drop it here.
    validations:
      required: false

  - type: textarea
    id: logs
    attributes:
      label: Logs
      description: Paste any logs or error messages here, or attach them to the issue.
    validations:
      required: false

  - type: textarea
    id: additional_context
    attributes:
      label: Additional Context
      description: Add any other relevant information or system configuration details.
    validations:
      required: false
