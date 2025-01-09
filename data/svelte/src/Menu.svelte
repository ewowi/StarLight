<script>
    import { fly, scale } from 'svelte/transition';
    import { quadOut } from 'svelte/easing';

    export let open;
</script>

{#if open}
    <div>
        {#each ['Home', 'Example', 'About', 'Contact'] as link, i}
            <p transition:fly={{ y: -15, delay: 50 * i }}>
                {link}
            </p>
        {/each}
    </div>

    <hr transition:scale={{ duration: 750, easing: quadOut, opacity: 1 }} />
{/if}

<style>
    :global(html) {
        background: #1d1d2f;
    }
    div {
        text-align: center;
        font-size: 1.5em;
        letter-spacing: 0.15em;
        padding: 1em;
        padding-top: 0;
        color: #eef;
    }
    p {
        cursor: pointer;
        width: max-content;
        margin: 1rem auto;
    }
    p:hover {
        text-decoration: underline;
    }
</style>
