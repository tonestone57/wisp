# LibDOM

LibDOM is an implementation of the W3C DOM API in C.

## API documentation
Currently, there is none. However, the code is well commented and the public API may be found in the "include" directory.

## Wisp Fork Status (July 2026)
Wisp maintains a diverged fork of LibDOM to support modern standards and QuickJS integration:
- **Mutation Hooks**: Native C-level hooks for observation of DOM mutations (ChildList, Attributes, CharacterData).
- **SVG Support**: Integrated support for SVG DOM interfaces.
- **QuickJS Integration**: Enhanced with structures to facilitate bridging to the QuickJS-ng engine.
