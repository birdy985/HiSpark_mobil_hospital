# Backend Development Guidelines

> Best practices for backend development in this project.

---

## Overview

This directory contains guidelines for backend development. In this repository, "backend" primarily means embedded C development against the WS63/fbb_ws63 SDK.

---

## Guidelines Index

| Guide | Description | Status |
|-------|-------------|--------|
| [WS63 SDK Source Map](./ws63-sdk-source-map.md) | Official docs, samples, and source lookup order for WS63 SDK work | Active |
| [WS63 SDK Coding Standard](./ws63-sdk-coding-standard.md) | Embedded C, SLE, driver, LiteOS, build, and validation rules | Active |
| [Directory Structure](./directory-structure.md) | Module organization and file layout | To fill |
| [Database Guidelines](./database-guidelines.md) | ORM patterns, queries, migrations | To fill |
| [Error Handling](./error-handling.md) | Error types, handling strategies | To fill |
| [Quality Guidelines](./quality-guidelines.md) | Code standards, forbidden patterns | To fill |
| [Logging Guidelines](./logging-guidelines.md) | Structured logging, log levels | To fill |

---

## Pre-Development Checklist

Before editing SDK code:

1. Read [WS63 SDK Source Map](./ws63-sdk-source-map.md).
2. Read [WS63 SDK Coding Standard](./ws63-sdk-coding-standard.md).
3. Search the official docs and local samples named by the source map for the exact feature being changed.
4. Search existing `CMakeLists.txt`, `Kconfig`, and `config.py` patterns before registering new code.
5. Identify the build command that will validate the change.

---

## How to Fill These Guidelines

For each guideline file:

1. Document your project's **actual conventions** (not ideals)
2. Include **code examples** from your codebase
3. List **forbidden patterns** and why
4. Add **common mistakes** your team has made

The goal is to help AI assistants and new team members understand how YOUR project works.

---

**Language**: All documentation should be written in **English**.
