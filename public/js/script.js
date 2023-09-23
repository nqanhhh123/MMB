document.addEventListener("DOMContentLoaded", function () {
  const navLinks = document.querySelectorAll(".js");
  const tabPanes = document.querySelectorAll(".tab-pane");
  // make tab1 active by default
  document.querySelector("#tab1").classList.add("active");
  navLinks.forEach(function (link) {
    link.addEventListener("click", function (e) {
      e.preventDefault();
      const target = this.getAttribute("href");

      navLinks.forEach(function (link) {
        link.classList.remove("active");
        link.classList.add("inActive");
        
      });

      tabPanes.forEach(function (pane) {
        pane.classList.remove("active");
        link.classList.add("inActive");
      });

      this.classList.add("active");
      document.querySelector(target).classList.add("active");
      this.classList.remove("inActive")
      document.querySelector(target).classList.remove("inActive");
    });
  });
});

const posts = document.querySelectorAll('.post');
const postNav = document.querySelector('#postNav');
const postNavExit = document.querySelector('#postNavExit');
const postNavBackground = document.querySelector('#postNavBackground');

posts.forEach(post => {
  post.addEventListener('click', () => {
    posts.forEach(p => {
      p.removeAttribute('title');
    });
    post.setAttribute('title', 'nowActive');
    postNav.setAttribute('title', 'nowActive');
  });
});

postNavExit.addEventListener('click', () => {
  posts.forEach(p => {
    p.removeAttribute('title');
  });
  postNav.removeAttribute('title');
});

postNavBackground.addEventListener('click', () => {
  posts.forEach(p => {
    p.removeAttribute('title');
  });
  postNav.removeAttribute('title');
});

// create an img auto slider
const imgSlider = document.querySelector('.img-slider');
const imgSliderItems = document.querySelectorAll('.img-slider-item');
const imgSliderNext = document.querySelector('.img-slider-next');
const imgSliderPrev = document.querySelector('.img-slider-prev');
const imgSliderDots = document.querySelector('.img-slider-dots');
const imgSliderDotsItems = document.querySelectorAll('.img-slider-dot');

