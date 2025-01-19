<!--
	Example by GHOST and Fractal
	GHOST: https://github.com/ghostdevv
	Fractal: https://github.com/FractalHQ
-->

<!--
	THIS REPL IS FOR svelte-hamburgers 2
	To view v3 check here: https://svelte.dev/repl/2339dbd1356a4149aabb17daa0a17e40
-->

<!-- StarBase: -->
<!-- https://www.npmjs.com/package/svelte-hamburgers?ref=madewithsvelte.com -->
<!-- https://svelte.dev/playground/86b10871cc7f42b39e74d71bdb4d643e?version=3.38.2 -->
 
<script>
    import {Hamburger} from 'svelte-hamburgers';
    import Menu from './Menu.svelte';

    let open = $state('foo');
</script>

<Hamburger
    bind:open
    color="white" />

<Menu bind:open />
