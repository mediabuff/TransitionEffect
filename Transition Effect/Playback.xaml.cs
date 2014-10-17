using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Transition_Effect.Common;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

using AdvancedMediaSource;
using Windows.Media.Core;
using Windows.Media.MediaProperties;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkID=390556

namespace Transition_Effect
{
    
    public sealed partial class Playback : Page
    {
        NavigationHelper navigationHelper;

        public Playback()
        {
            LoadVideos();
            LoadEffects();

            this.InitializeComponent();

            this.DataContext = this;
            this.NavigationCacheMode = NavigationCacheMode.Required;
            navigationHelper = new NavigationHelper(this);
        }

        private void Apply_Effect(object sender, RoutedEventArgs e)
        {

        }

        private void LoadEffects()
        {
            EffectList = new ObservableCollection<Effect>();
            EffectList.Add(new Effect() { Name = "Black", EffectType = TransitionEffectType.TRANSITION_BLACK });
            EffectList.Add(new Effect() { Name = "White", EffectType = TransitionEffectType.TRANSITION_WHITE });
            EffectList.Add(new Effect() { Name = "Black Fading", EffectType = TransitionEffectType.TRANSITION_FADE_TO_BLACK });
            EffectList.Add(new Effect() { Name = "White Fading", EffectType = TransitionEffectType.TRANSITION_FADE_TO_WHITE });
            EffectList.Add(new Effect() { Name = "Fading Transit", EffectType = TransitionEffectType.TRANSITION_FADE_OVERLAP });
            EffectList.Add(new Effect() { Name = "Right Left", EffectType = TransitionEffectType.TRANSITION_RIGHT_TO_LEFT });
            EffectList.Add(new Effect() { Name = "Top Bottom", EffectType = TransitionEffectType.TRANSITION_TOP_TO_BOTTOM });
            EffectList.Add(new Effect() { Name = "Bottom Top", EffectType = TransitionEffectType.TRANSITION_BOTTOM_TO_TOP });
            EffectList.Add(new Effect() { Name = "ZoomIn", EffectType = TransitionEffectType.TRANSITION_ROOM_IN });
            EffectList.Add(new Effect() { Name = "ZoomOut", EffectType = TransitionEffectType.TRANSITION_ROOM_OUT });
        }

        private void LoadVideos()
        {
            VideoList = new ObservableCollection<string>();

            VideoList.Add(@"ms-appx:///Assets/Videos/1.mp4");
            VideoList.Add(@"ms-appx:///Assets/Videos/big_buck_bunny_trailer_480p_high.mp4");
            VideoList.Add(@"ms-appx:///Assets/Videos/Mortal Kombat Legacy.mp4");

        }

        public string CurrentVideo { get; set; }
        public string SecondVideo { get; set; }
        public string ThirdVideo { get; set; }
        public Effect CurrentEffect { get; set; }

        public ObservableCollection<string> VideoList { get; set; }
        public ObservableCollection<Effect> EffectList { get; set; }

        private void Save_click(object sender, RoutedEventArgs e)
        {
            
        }

        private void OnPropertyChanged(string p)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(p));
        }


        public event PropertyChangedEventHandler PropertyChanged;

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            this.navigationHelper.OnNavigatedTo(e);
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            this.navigationHelper.OnNavigatedFrom(e);
        }

        private void Video_MediaFailed(object sender, ExceptionRoutedEventArgs e)
        {
            throw new NotSupportedException(e.ErrorMessage);
        }
    }
}
