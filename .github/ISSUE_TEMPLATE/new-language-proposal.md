---
name: New language proposal
about: Propose adding an implementation of Alchemy Logging in a new language
title: ''
labels: ''
assignees: ''

---

## Use Case Overview

Please describe which language you are proposing support for, and the use case for the project where you would like to use Alchemy Logging.

## API Overview

In this section, please outline how a user will leverage the features of the [Implementation Spec](../../docs/implementation-spec.md#1-interface).

### Required Features

* **Core Singleton**:
    * **Channels** ([reference](../../docs/implementation-spec.md#channels-and-levels)):
    * **Levels** ([reference](../../docs/implementation-spec.md#channels-and-levels)):
    * **Single-call global configuration** ([reference](../../docs/implementation-spec.md#11-configuration)):
* **Formatting**:
    * **Pretty-print formatting** ([reference](../../docs/implementation-spec.md#14-pretty-formatting)):
    * **Json formatting** ([reference](../../docs/implementation-spec.md#15-json-formatting)):
* **Usage**:
    * **Individual Records** ([reference](../../docs/implementation-spec.md#131-individual-text-records)):
    * **Shared Channels** ([reference](../../docs/implementation-spec.md#133-channel-log-objects-and-functions)):

### Optional Features

* **Per-message map data** ([reference](../../docs/implementation-spec.md#132-map-records)):
* **Thread IDs** ([reference](../../docs/implementation-spec.md#11-configuration)):
* **Thread log function** ([reference](../../docs/implementation-spec.md#161-thread-logs)):
* **Metadata** ([reference](../../docs/implementation-spec.md#162-metadata)):
* **Log scopes** ([reference](../../docs/implementation-spec.md#163-scoped-logs)):
* **Function scopes** ([reference](../../docs/implementation-spec.md#164-function-trace-scopes)):
* **Timed scopes** ([reference](../../docs/implementation-spec.md#165-timed-scopes)):

## Implementation Details

In this section, please outline how you plan to implement the core elements of the [Implementation Spec](../../docs/implementation-spec.md#2-implementation).

### Required Implementaiton Details

* **Core Singleton** ([reference](../../docs/implementation-spec.md#21-core)):
* **Lazy Message Construction** ([reference](../../docs/implementation-spec.md#22-lazy-message-construction)):
* **Thread Safety** ([reference](../../docs/implementation-spec.md#23-thread-safety))

### Optional Implementation Details

* **Dynamic Configureation** ([reference](../../docs/implementation-spec.md#24-dynamic-configuration))

### Dependencies

Please indicate any/all non-default dependencies that this implementation will need.

## Alternatives

In this section, please provide details about alternate approaches that could be taken and why the proposed approach is the right choice.

### Other Common Logging Packages

List the most commonly used logging packages in the target language and how they differ from `Alchemy Logging`.

### Open Questions

List out any open questions you'd like to work through in the course of the proposal.
