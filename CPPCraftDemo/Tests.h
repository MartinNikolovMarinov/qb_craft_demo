#pragma once

void beforeTests();

void runFunctionalTests();

template <size_t TCount>
void runPerfTestFindMatchingIn();

void runAllTestCases();