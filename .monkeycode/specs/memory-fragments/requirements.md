# Requirements Document

## Introduction

"记忆碎片" (Memory Fragments) is a feature that allows users to save, view, and edit terminal session knowledge snippets. Users can capture selected terminal output, clipboard content, or manually typed notes, with full context tracking including terminal type, working directory, executed commands, and session metadata.

## Glossary

- **Memory Fragment**: A saved knowledge snippet with content, context metadata, and source information
- **Context Metadata**: Information about the terminal state when the fragment was saved (terminal type, working directory, session ID, command history)
- **Source**: How the fragment was created (selection, clipboard, manual entry)

## Requirements

### Requirement 1: Save Selection as Memory Fragment

**User Story:** AS a terminal user, I want to save selected text as a memory fragment, so that I can capture important terminal output for future reference.

#### Acceptance Criteria

1. WHEN user selects text in the terminal pane, the context menu SHALL include "保存为记忆碎片"
2. WHEN user clicks "保存为记忆碎片", the system SHALL save the selected text as a memory fragment
3. WHEN saving a memory fragment, the system SHALL capture the following context:
   - Terminal type (bash, zsh, cmd, etc.)
   - Working directory
   - Session identifier
   - Command history (last 10 commands)
   - Timestamp
4. AFTER saving, the system SHALL show a brief confirmation notification

### Requirement 2: Create Memory from Clipboard

**User Story:** AS a terminal user, I want to create a memory fragment from clipboard content, so that I can save externally copied knowledge.

#### Acceptance Criteria

1. WHEN user right-clicks without selecting text, the context menu SHALL include "剪切板粘贴为记忆碎片"
2. WHEN user clicks "剪切板粘贴为记忆碎片", the system SHALL open the memory editor with clipboard content pre-filled
3. WHILE the memory editor is open, the system SHALL display the context metadata for editing
4. WHEN user saves the memory editor, the system SHALL create a memory fragment with the entered content and metadata

### Requirement 3: Create New Memory Fragment Manually

**User Story:** AS a terminal user, I want to manually create a memory fragment, so that I can record notes and knowledge not present in the terminal.

#### Acceptance Criteria

1. WHEN user right-clicks without selecting text, the context menu SHALL include "新增记忆碎片"
2. WHEN user clicks "新增记忆碎片", the system SHALL open an empty memory editor dialog
3. WHEN user saves the memory editor, the system SHALL create a memory fragment with the entered content and current session context

### Requirement 4: View Memory Fragments History

**User Story:** AS a terminal user, I want to view my saved memory fragments in reverse chronological order, so that I can quickly find recent knowledge.

#### Acceptance Criteria

1. WHEN user opens the memory viewer, the system SHALL display fragments sorted by timestamp in descending order
2. FOR each memory fragment entry, the system SHALL display:
   - Content preview (first 3 lines)
   - Creation timestamp
   - Terminal type
   - Working directory
   - Source indicator (selection/clipboard/manual)
3. WHEN user clicks a memory fragment, the system SHALL show the full content with all metadata

### Requirement 5: Edit and Delete Memory Fragments

**User Story:** AS a terminal user, I want to edit or delete existing memory fragments, so that I can maintain my knowledge base.

#### Acceptance Criteria

1. WHEN user views a memory fragment, the system SHALL provide "编辑" and "删除" actions
2. WHEN user edits a memory fragment, the system SHALL open the memory editor with existing content
3. WHEN user deletes a memory fragment, the system SHALL request confirmation before deletion
4. AFTER deletion, the system SHALL remove the fragment from storage and update the view

### Requirement 6: Memory Fragment Persistence

**User Story:** AS a terminal user, I want my memory fragments to persist across sessions, so that I can access them later.

#### Acceptance Criteria

1. WHEN a memory fragment is created, the system SHALL save it to persistent storage
2. WHEN the application starts, the system SHALL load all existing memory fragments
3. WHEN a memory fragment is edited, the system SHALL update the persistent storage
