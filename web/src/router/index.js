import { createRouter, createWebHistory } from 'vue-router'
import LandingView from '../views/LandingView.vue'
import DocsView from '../views/DocsView.vue'
import DocsHome from '../views/docs/DocsHome.vue'
import BeginnersDoc from '../views/docs/BeginnersDoc.vue'
import InstallDoc from '../views/docs/InstallDoc.vue'
import SyntaxDoc from '../views/docs/SyntaxDoc.vue'
import TypesDoc from '../views/docs/TypesDoc.vue'
import StatementsDoc from '../views/docs/StatementsDoc.vue'
import ExpressionsDoc from '../views/docs/ExpressionsDoc.vue'
import CompilerDoc from '../views/docs/CompilerDoc.vue'

const routes = [
  { path: '/', name: 'home', component: LandingView },
  {
    path: '/docs',
    component: DocsView,
    children: [
      { path: '', name: 'docs', component: DocsHome },
      { path: 'beginners', name: 'docs-beginners', component: BeginnersDoc },
      { path: 'install', name: 'docs-install', component: InstallDoc },
      { path: 'syntax', name: 'docs-syntax', component: SyntaxDoc },
      { path: 'types', name: 'docs-types', component: TypesDoc },
      { path: 'statements', name: 'docs-statements', component: StatementsDoc },
      { path: 'expressions', name: 'docs-expressions', component: ExpressionsDoc },
      { path: 'compiler', name: 'docs-compiler', component: CompilerDoc },
    ],
  },
]

export default createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes,
  scrollBehavior() {
    return { top: 0 }
  },
})
