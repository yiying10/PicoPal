#include <assert.h>
#include <stdio.h>

#include "picopal_pico_state_model.h"

#define SECOND_US 1000000ULL

static void reaction_expires_without_changing_base(void)
{
    picopal_pico_state_model_t pico;
    picopal_pico_state_model_init(&pico);
    picopal_pico_state_model_set_base(&pico, PICOPAL_PICO_BASE_CODING);

    assert(picopal_pico_state_model_start_reaction(
        &pico,
        PICOPAL_PICO_REACTION_HAPPY,
        0
    ));
    assert(!picopal_pico_state_model_update(&pico, SECOND_US - 1));
    assert(picopal_pico_state_model_update(&pico, SECOND_US));
    assert(pico.reaction == PICOPAL_PICO_REACTION_NONE);
    assert(pico.base == PICOPAL_PICO_BASE_CODING);
}

static void higher_priority_interrupts_lower_priority(void)
{
    picopal_pico_state_model_t pico;
    picopal_pico_state_model_init(&pico);

    assert(picopal_pico_state_model_start_reaction(
        &pico,
        PICOPAL_PICO_REACTION_HAPPY,
        0
    ));
    assert(picopal_pico_state_model_start_reaction(
        &pico,
        PICOPAL_PICO_REACTION_CODEX_DONE,
        100
    ));
    assert(pico.reaction == PICOPAL_PICO_REACTION_CODEX_DONE);
}

static void lower_priority_cannot_replace_higher_priority(void)
{
    picopal_pico_state_model_t pico;
    picopal_pico_state_model_init(&pico);

    assert(picopal_pico_state_model_start_reaction(
        &pico,
        PICOPAL_PICO_REACTION_CODEX_DONE,
        0
    ));
    assert(!picopal_pico_state_model_start_reaction(
        &pico,
        PICOPAL_PICO_REACTION_SURPRISED,
        100
    ));
    assert(pico.reaction == PICOPAL_PICO_REACTION_CODEX_DONE);
}

static void error_rejects_temporary_reactions(void)
{
    picopal_pico_state_model_t pico;
    picopal_pico_state_model_init(&pico);
    picopal_pico_state_model_set_base(&pico, PICOPAL_PICO_BASE_ERROR);

    assert(!picopal_pico_state_model_start_reaction(
        &pico,
        PICOPAL_PICO_REACTION_CODEX_DONE,
        0
    ));
    assert(pico.reaction == PICOPAL_PICO_REACTION_NONE);
}

static void same_priority_reaction_is_rejected(void)
{
    picopal_pico_state_model_t pico;
    picopal_pico_state_model_init(&pico);

    assert(picopal_pico_state_model_start_reaction(
        &pico,
        PICOPAL_PICO_REACTION_HAPPY,
        0
    ));
    assert(!picopal_pico_state_model_start_reaction(
        &pico,
        PICOPAL_PICO_REACTION_DIZZY,
        0
    ));
    assert(pico.reaction == PICOPAL_PICO_REACTION_HAPPY);
}

int main(void)
{
    reaction_expires_without_changing_base();
    higher_priority_interrupts_lower_priority();
    lower_priority_cannot_replace_higher_priority();
    error_rejects_temporary_reactions();
    same_priority_reaction_is_rejected();
    puts("pico_state_model_test: all tests passed");
    return 0;
}
