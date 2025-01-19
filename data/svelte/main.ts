import { mount } from 'svelte'
import App from './src/App.svelte'

const app = mount(App, {
  target: document.getElementById('app')!,
})

export default app
